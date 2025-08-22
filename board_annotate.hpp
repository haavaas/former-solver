

#define R 1
#define G 2
#define B 3
#define Y 4

#define HSIZE 7
#define VSIZE 9

namespace Annotator {

struct Params {
    int rows = VSIZE;         // grid height
    int cols = HSIZE;         // grid width
    double innerRatio = 0.60; // central crop ratio for color sampling
    double satMin = 0.50;     // HSV S threshold to ignore background
    double valMin = 0.50;     // HSV V threshold to ignore background
};

std::vector<std::vector<uint8_t>> analyzeBoard(const std::string& imagePath, Params P);
} // namespace Annotator
