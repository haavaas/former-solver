// board_annotate.cpp
#include <opencv2/opencv.hpp>
#include <cmath>
#include <string>
#include <vector>
#include <iostream>
#include <numeric>
#include "board_annotate.hpp"

namespace Annotator {

// ----- Hue utilities -----
static inline double circularMeanDegrees(const std::vector<double>& degs) {
    if (degs.empty()) return 0.0;
    double sx = 0.0, sy = 0.0;
    for (double d : degs) {
        double r = d * CV_PI / 180.0;
        sx += std::cos(r);
        sy += std::sin(r);
    }
    double ang = std::atan2(sy, sx);
    if (ang < 0) ang += 2 * CV_PI;
    return ang * 180.0 / CV_PI;
}

static inline uint8_t hueToLetter(double deg) {
    // Primary bands
    if (deg >= 310 || deg < 15)   return R;  // Pink
    if (deg >= 15  && deg < 55)   return Y;  // Orange
    if (deg >= 80  && deg < 165)  return G;  // Green
    if (deg >= 175 && deg < 220)  return B;  // Blue
    // Fallback to nearest prototype
    const std::vector<std::pair<uint8_t,double>> proto = {
        {R, 340}, {Y, 30}, {G, 130}, {B, 200}
    };
    auto cyc = [](double a, double b) {
        double d = std::fmod(std::abs(a - b), 360.0);
        return std::min(d, 360.0 - d);
    };
    uint8_t best = R; double bestD = 1e9;
    for (auto& p : proto) {
        double d = cyc(deg, p.second);
        if (d < bestD) { bestD = d; best = p.first; }
    }
    return best;
}

// ----- Core: analyze one board -----
std::vector<std::vector<uint8_t>> analyzeBoard(
    const cv::Mat& bgr, const Params& P)
{
    CV_Assert(!bgr.empty());
    const int rows = P.rows, cols = P.cols;

    const double cellW = static_cast<double>(bgr.cols) / cols;
    const double cellH = static_cast<double>(bgr.rows) / rows;

    std::vector<std::vector<uint8_t>> labels(rows, std::vector<uint8_t>(cols, 0));

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int x0 = static_cast<int>(std::floor(c * cellW));
            int y0 = static_cast<int>(std::floor(r * cellH));
            int x1 = static_cast<int>(std::floor((c + 1) * cellW));
            int y1 = static_cast<int>(std::floor((r + 1) * cellH));

            // inner crop for color sampling
            int iw = x1 - x0, ih = y1 - y0;
            int ix0 = x0 + static_cast<int>((1.0 - P.innerRatio) * 0.5 * iw);
            int iy0 = y0 + static_cast<int>((1.0 - P.innerRatio) * 0.5 * ih);
            int ix1 = x1 - static_cast<int>((1.0 - P.innerRatio) * 0.5 * iw);
            int iy1 = y1 - static_cast<int>((1.0 - P.innerRatio) * 0.5 * ih);

            cv::Rect roi(std::max(0, ix0), std::max(0, iy0),
                         std::max(1, ix1 - ix0), std::max(1, iy1 - iy0));
            roi &= cv::Rect(0,0,bgr.cols,bgr.rows);

            cv::Mat patchBGR = bgr(roi);
            cv::Mat patchHSV;
            cv::cvtColor(patchBGR, patchHSV, cv::COLOR_BGR2HSV);

            std::vector<cv::Mat> ch; cv::split(patchHSV, ch); // H,S,V (H: 0..179)
            const double Sthr = P.satMin * 255.0;
            const double Vthr = P.valMin * 255.0;
            cv::Mat mask = (ch[1] > Sthr) & (ch[2] > Vthr);

            std::vector<double> huesDeg;
            huesDeg.reserve(static_cast<size_t>(mask.total()));

            for (int y = 0; y < mask.rows; ++y) {
                const uchar* mptr = mask.ptr<uchar>(y);
                const uchar* hptr = ch[0].ptr<uchar>(y);
                for (int x = 0; x < mask.cols; ++x) {
                    if (mptr[x]) {
                        // OpenCV hue 0..179 maps to 0..360
                        huesDeg.push_back(hptr[x] * 2.0);
                    }
                }
            }

            double hueDeg;
            if (huesDeg.empty()) {
                // Fallback: mean color of the patch
                cv::Scalar meanBGR = cv::mean(patchBGR);
                cv::Mat one(1,1,CV_8UC3, cv::Vec3b(
                    (uchar)std::clamp((int)std::round(meanBGR[0]), 0, 255),
                    (uchar)std::clamp((int)std::round(meanBGR[1]), 0, 255),
                    (uchar)std::clamp((int)std::round(meanBGR[2]), 0, 255)
                ));
                cv::Mat oneHSV;
                cv::cvtColor(one, oneHSV, cv::COLOR_BGR2HSV);
                hueDeg = oneHSV.at<cv::Vec3b>(0,0)[0] * 2.0;
            } else {
                hueDeg = circularMeanDegrees(huesDeg);
            }

            labels[r][c] = hueToLetter(hueDeg);
        }
    }
    return labels;
}

// ----- Centroid of shape inside a cell -----
static inline cv::Point2d cellCentroid(const cv::Mat& cellBGR, double satMin, double valMin) {
    cv::Mat hsv; cv::cvtColor(cellBGR, hsv, cv::COLOR_BGR2HSV);
    std::vector<cv::Mat> ch; cv::split(hsv, ch);
    cv::Mat mask = (ch[1] > satMin * 255.0) & (ch[2] > valMin * 255.0);
    cv::Moments m = cv::moments(mask, true);
    if (m.m00 > 1e-6) {
        return cv::Point2d(m.m10 / m.m00, m.m01 / m.m00);
    } else {
        return cv::Point2d(cellBGR.cols * 0.5, cellBGR.rows * 0.5);
    }
}

std::vector<std::vector<uint8_t>> analyzeBoard(const std::string& imagePath, Params P){
    cv::Mat bgr = cv::imread(imagePath, cv::IMREAD_COLOR);
    if (bgr.empty()) {
        throw std::runtime_error("Failed to read image: " + imagePath);
    }
    return analyzeBoard(bgr, P);
}

// ----- Draw outlined text (stroke + fill) -----
static inline void drawCenteredLetter(cv::Mat& img, const std::string& letter,
                                      cv::Point2d center, double cellSize,
                                      const cv::Scalar& fill = {255,255,255},
                                      const cv::Scalar& stroke = {20,0,40})
{
    int fontFace = cv::FONT_HERSHEY_SIMPLEX;
    double fontScale = std::max(0.4, cellSize * 0.0025); // scale by cell size
    int strokeThickness = std::max(2, (int)std::round(cellSize * 0.07));
    int fillThickness   = std::max(1, strokeThickness - 2);
    int baseline = 0;
    cv::Size textSize = cv::getTextSize(letter, fontFace, fontScale, strokeThickness, &baseline);
    cv::Point org((int)std::round(center.x - textSize.width*0.5),
                  (int)std::round(center.y + textSize.height*0.5));

    // Stroke (draw slightly thicker and darker)
    cv::putText(img, letter, org, fontFace, fontScale, stroke, strokeThickness, cv::LINE_AA);
    // Fill (on top)
    cv::putText(img, letter, org, fontFace, fontScale, fill,   fillThickness,   cv::LINE_AA);
}

// ----- Render labels on the board -----
std::string renderLabelsOnBoard(const cv::Mat& bgr,
                                const std::vector<std::vector<uint8_t>>& labels,
                                const std::string& outPath,
                                bool drawGrid = false,
                                const Params& P = Params{})
{
    CV_Assert(!bgr.empty());
    int rows = (int)labels.size();
    int cols = rows ? (int)labels[0].size() : 0;
    CV_Assert(rows == P.rows && cols == P.cols);

    cv::Mat canvas = bgr.clone();

    const double cellW = (double)canvas.cols / cols;
    const double cellH = (double)canvas.rows / rows;
    const double cellMin = std::min(cellW, cellH);

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int x0 = (int)std::floor(c * cellW);
            int y0 = (int)std::floor(r * cellH);
            int x1 = (int)std::floor((c + 1) * cellW);
            int y1 = (int)std::floor((r + 1) * cellH);

            cv::Rect cellRect(x0, y0, std::max(1, x1 - x0), std::max(1, y1 - y0));
            cv::Mat cell = canvas(cellRect);

            // Find centroid of colored shape
            cv::Point2d localC = cellCentroid(cell, P.satMin, P.valMin);
            cv::Point2d globalC(x0 + localC.x, y0 + localC.y);

            // Draw letter
            std::string ch(1, labels[r][c]);
            drawCenteredLetter(canvas, ch, globalC, cellMin);

            if (drawGrid) {
                cv::rectangle(canvas, cellRect, cv::Scalar(255,255,255), 2, cv::LINE_AA);
            }
        }
    }

    if (!outPath.empty()) {
        cv::imwrite(outPath, canvas);
    }
    return outPath;
}

// ----- Convenience: analyze + annotate -----
std::pair<std::vector<std::vector<uint8_t>>, std::string>
analyzeAndAnnotate(const std::string& imagePath,
                   const std::string& outPath,
                   const Params& P = Params{},
                   bool drawGrid = false)
{
    cv::Mat bgr = cv::imread(imagePath, cv::IMREAD_COLOR);
    if (bgr.empty()) {
        throw std::runtime_error("Failed to read image: " + imagePath);
    }
    auto labels = analyzeBoard(bgr, P);
    renderLabelsOnBoard(bgr, labels, outPath, drawGrid, P);
    return {labels, outPath};
}

} // namespace Annotator

// -------- Example CLI usage --------
#ifdef BOARD_ANNOTATE_MAIN
int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input_image> <output_image> [rows=9] [cols=7]\n";
        return 1;
    }
    std::string in  = argv[1];
    std::string out = argv[2];
    Annotator::Params P;
    if (argc >= 4) P.rows = std::stoi(argv[3]);
    if (argc >= 5) P.cols = std::stoi(argv[4]);

    try {
        auto res = Annotator::analyzeAndAnnotate(in, out, P, /*drawGrid=*/false);
        const auto& labels = res.first;
        std::cout << "Matrix (rows x cols):\n";
        for (const auto& row : labels) {
            for (size_t i = 0; i < row.size(); ++i) {
                std::cout << row[i] << (i + 1 == row.size() ? '\n' : ' ');
            }
        }
        std::cout << "Saved overlay: " << res.second << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
    return 0;
}
#endif
 