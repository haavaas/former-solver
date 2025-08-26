#include "cli.hpp"

#include <charconv>
#include <iostream>
#include <optional>
#include <string_view>

using std::string_view;

namespace
{
    std::optional<int> parse_int(string_view s)
    {
        int v{};
        const char *first = s.data();
        const char *last = s.data() + s.size();
        auto [p, ec] = std::from_chars(first, last, v);
        if (ec != std::errc{} || p != last)
            return std::nullopt; // disallow partials/garbage
        return v;
    }
}

void print_help(const char *prog)
{
    std::cout << "Usage:\n"
          "  "
       << prog << " [search_width] [threads]\n"
                  "  "
       << prog << " -w <search_width> -t <threads>\n"
                  "  "
       << prog << " --help\n\n"
                  "Arguments:\n"
                  "  [search_width]         Optional. Integer > 0. Defaults to 1000.\n"
                  "  [threads]              Optional. Integer > 0. Defaults to 4.\n\n"
                  "Options:\n"
                  "  -w, --width <int>      Search/beam width.\n"
                  "  -t, --threads <int>    Number of worker threads.\n"
                  "  -h, --help             Show this help and exit.\n\n"
                  "Examples:\n"
                  "  "
       << prog << "\n"
                  "  "
       << prog << " 5000 8\n"
                  "  "
       << prog << " -w 5000 -t 8\n";
}

ParseResult parse_cli(int argc, char **argv)
{
    ParseResult result;

    SolverSettings cfg; // start with defaults
    bool width_set = false;
    bool threads_set = false;

    for (int i = 1; i < argc; ++i)
    {
        string_view arg(argv[i]);

        if (arg == "-h" || arg == "--help")
        {
            result.show_help = true;
            result.ok = false; // caller should bail after showing help
            result.exit_code = 0;
            return result;
        }
        else if (arg == "-w" || arg == "--width")
        {
            if (i + 1 >= argc)
            {
                result.error = "Error: Missing value after " + std::string(arg);
                return result;
            }
            string_view val(argv[++i]);
            auto p = parse_int(val);
            if (!p)
            {
                result.error = "Error: Invalid integer for width: " + std::string(val);
                return result;
            }
            cfg.search_width = *p;
            width_set = true;
        }
        else if (arg == "-t" || arg == "--threads")
        {
            if (i + 1 >= argc)
            {
                result.error = "Error: Missing value after " + std::string(arg);
                return result;
            }
            string_view val(argv[++i]);
            auto p = parse_int(val);
            if (!p)
            {
                result.error = "Error: Invalid integer for threads: " + std::string(val);
                return result;
            }
            cfg.threads = *p;
            threads_set = true;
        }
        else if (!arg.empty() && arg.front() != '-')
        {
            // Positional handling: [width] [threads]
            if (!width_set)
            {
                auto p = parse_int(arg);
                if (!p)
                {
                    result.error = "Error: Expected integer for search_width, got: " + std::string(arg);
                    return result;
                }
                cfg.search_width = *p;
                width_set = true;
            }
            else if (!threads_set)
            {
                auto p = parse_int(arg);
                if (!p)
                {
                    result.error = "Error: Expected integer for threads, got: " + std::string(arg);
                    return result;
                }
                cfg.threads = *p;
                threads_set = true;
            }
            else
            {
                result.error = "Error: Too many positional arguments.";
                return result;
            }
        }
        else
        {
            result.error = "Error: Unknown option: " + std::string(arg);
            return result;
        }
    }

    // Validation
    if (cfg.search_width <= 0)
    {
        result.error = "Error: search_width must be > 0 (got " + std::to_string(cfg.search_width) + ").";
        return result;
    }
    if (cfg.threads <= 0)
    {
        result.error = "Error: threads must be > 0 (got " + std::to_string(cfg.threads) + ").";
        return result;
    }

    result.ok = true;
    result.settings = std::move(cfg);
    result.exit_code = 0;
    return result;
}
