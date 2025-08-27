#include <array>
#include <string>
#include <cstdint>
#include <optional>
#include <curl/curl.h>
#include "grid.hpp"
#include "cmath"
#include <stdexcept>
#include <string_view>
#include <openssl/evp.h>
#include <chrono>
#include <ctime>




class Alea {
    // --- Mash 0.9 (exact JS semantics) ---
    struct Mash {
        double s = 4022871197.0; // keep as double, not uint32_t

        double operator()(const std::string& data) {
            for (unsigned char ch : data) {
                s += ch;
                double d = 0.02519603282416938 * s;

                // s = d >>> 0  (coerce to uint32, stored as double)
                s = static_cast<double>(static_cast<uint32_t>(d));
                d -= s;

                d *= s;
                // s = d >>> 0
                s = static_cast<double>(static_cast<uint32_t>(d));
                d -= s;

                // s += d * 2^32  (DO NOT mask/truncate here)
                s += d * 4294967296.0; // 2^32
            }

            // return (s >>> 0) * 2^-32
            // emulate '>>> 0' safely with fmod (avoid UB on overflow casts)
            double u32 = std::fmod(s, 4294967296.0);
            if (u32 < 0) u32 += 4294967296.0; // just in case
            uint32_t u = static_cast<uint32_t>(u32);
            return static_cast<double>(u) * 2.3283064365386963e-10; // 2^-32
        }
    };

    // Alea state
    double s0{}, s1{}, s2{};
    uint32_t c {1};

public:
    // Single-string seed (matches your usage)
    explicit Alea(const std::string& seed) {
        Mash mash;
        s0 = mash(" ");
        s1 = mash(" ");
        s2 = mash(" ");

        s0 -= mash(seed);
        if (s0 < 0.0) s0 += 1.0;

        s1 -= mash(seed);
        if (s1 < 0.0) s1 += 1.0;

        s2 -= mash(seed);
        if (s2 < 0.0) s2 += 1.0;
    }

    // Optional: multi-part seeding, like JS t('a','b',123)
    explicit Alea(const std::vector<std::string>& seeds) {
        Mash mash;
        s0 = mash(" ");
        s1 = mash(" ");
        s2 = mash(" ");
        for (const auto& seed : seeds) {
            s0 -= mash(seed);
            if (s0 < 0.0) s0 += 1.0;
            s1 -= mash(seed);
            if (s1 < 0.0) s1 += 1.0;
            s2 -= mash(seed);
            if (s2 < 0.0) s2 += 1.0;
        }
    }

    // Core step: r = p - (c = (p | 0)), p = 2091639*s0 + c*2^-32
    double next() {
        const double p = 2091639.0 * s0 + static_cast<double>(c) * 2.3283064365386963e-10;
        s0 = s1;
        s1 = s2;

        const uint32_t ip = static_cast<uint32_t>(p); // p is small and >= 0, so trunc == floor
        c = ip;
        s2 = p - static_cast<double>(ip); // fractional part in [0,1)

        return s2;
    }
};

inline Grid generateBoard(const std::string &seed)
{
    Alea rng(seed);
    static constexpr Grid::Cell lut[4] = {
        Grid::Cell::Orange,
        Grid::Cell::Pink,
        Grid::Cell::Green,
        Grid::Cell::Blue 
    };

    Grid grid;
    for (int i = 0; i < Grid::height; ++i)
    {
        for (int j = 0; j < Grid::width; ++j)
        {
            const int idx = static_cast<int>(rng.next() * 4.0); // floor
            grid.at(i, j) = lut[idx];
        }
    }
    return grid;
}

namespace http
{
    static size_t writeToString(void *ptr, size_t size, size_t nmemb, void *userdata)
    {
        auto *out = static_cast<std::string *>(userdata);
        out->append(static_cast<const char *>(ptr), size * nmemb);
        return size * nmemb;
    }

    inline std::optional<std::string> get(const std::string &url, long timeoutSeconds = 6)
    {
        CURL *curl = curl_easy_init();
        if (!curl)
            return std::nullopt;

        std::string buffer;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSeconds);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeoutSeconds);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "former-seed-fetch/1.0");

        CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK || http_code != 200)
            return std::nullopt;
        return buffer;
    }
}

// Minimal JSON extraction: finds `"seed":{"value":"<string>"}`
inline std::optional<std::string> extract_seed_value(const std::string &json)
{
    std::size_t pos = json.find("\"seed\"");
    if (pos == std::string::npos)
        return std::nullopt;

    pos = json.find("\"value\"", pos);
    if (pos == std::string::npos)
        return std::nullopt;

    pos = json.find('"', json.find(':', pos));
    if (pos == std::string::npos)
        return std::nullopt;

    std::size_t start = pos + 1;
    std::size_t end = json.find('"', start);
    if (end == std::string::npos || end <= start)
        return std::nullopt;

    return json.substr(start, end - start);
}

// Convenience function to fetch today’s seed; returns std::nullopt on failure
inline std::optional<std::string> fetch_today_seed()
{
    static const std::string kUrl =
        "https://www.nrk.no/konkurranse/api/v1/minispill/former/seed";
    auto body = http::get(kUrl);
    if (!body)
        return std::nullopt;
    return extract_seed_value(*body);
}


#include <openssl/evp.h>  // EVP_Q_digest

namespace crypto {

// Returns lowercase hex (32 chars) for the MD5 of `data`.
inline std::string md5_hex(std::string_view data) {
    std::array<std::uint8_t, 16> digest{};
    size_t out_len = 0;

    const auto* p = reinterpret_cast<const unsigned char*>(data.data());
    if (EVP_Q_digest(nullptr, "MD5", nullptr, p, data.size(),
                     reinterpret_cast<unsigned char*>(digest.data()),
                     &out_len) != 1 || out_len != digest.size()) {
        throw std::runtime_error("EVP_Q_digest(MD5) failed");
    }

    // Convert to lowercase hex without iostream overhead
    static constexpr char kHex[] = "0123456789abcdef";
    std::string hex(32, '\0');
    for (size_t i = 0; i < digest.size(); ++i) {
        const auto b = digest[i];
        hex[2 * i]     = kHex[b >> 4];
        hex[2 * i + 1] = kHex[b & 0x0F];
    }
    return hex;
}

} // namespace crypto


std::string date_minus_31_days() {
    using namespace std::chrono;

    // Get current time and subtract 31 days
    auto now = system_clock::now() - days{31};

    // Convert to calendar time
    std::time_t t = system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);

    // Build string: day + month + year (no leading zeros)
    return std::to_string(tm.tm_mday) +
           std::to_string(tm.tm_mon + 1) +
           std::to_string(tm.tm_year + 1900);
}


Grid fetch_grid(void) {
    auto s = fetch_today_seed();
    if (!s) {
        std::cerr << "Failed to fetch today's seed\n";
        return {};
    }

    auto date = date_minus_31_days();
    auto seed = crypto::md5_hex("2772025");
    
    std::cout << "Date 31 days ago: " << date << "\n";
    std::cout << "Today's seed: " << *s << "\n";
    std::cout << "    Our seed: " << seed << "\n";
    return generateBoard(*s);
}
