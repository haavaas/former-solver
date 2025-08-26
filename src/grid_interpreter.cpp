#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "grid.hpp"
#include <iostream>
#include <string>
#include <cmath>
#include <algorithm>

static void rgb_to_hsv(int r, int g, int b, double &h, double &s, double &v)
{
    double rf = r / 255.0, gf = g / 255.0, bf = b / 255.0;
    double max = std::max({rf, gf, bf});
    double min = std::min({rf, gf, bf});
    v = max;

    double d = max - min;
    s = max == 0 ? 0 : d / max;

    if (d == 0)
    {
        h = 0;
    }
    else if (max == rf)
    {
        h = 60 * std::fmod(((gf - bf) / d), 6);
    }
    else if (max == gf)
    {
        h = 60 * (((bf - rf) / d) + 2);
    }
    else
    {
        h = 60 * (((rf - gf) / d) + 4);
    }
    if (h < 0)
        h += 360;
}

static Grid::Cell identify_shape_hsv(double h, double s, double v)
{
    // Pink Circle
    if ((h > 290 && h < 350) && s > 0.3 && v > 0.5)
        return Grid::Cell::Pink;
    // Orange Parallelogram
    if ((h > 15 && h < 45) && s > 0.3 && v > 0.5)
        return Grid::Cell::Orange;
    // Green Arrow
    if ((h > 70 && h < 170) && s > 0.3 && v > 0.3)
        return Grid::Cell::Green;
    // Blue Square
    if ((h > 180 && h < 260) && s > 0.3 && v > 0.3)
        return Grid::Cell::Blue;
    return Grid::Cell::Empty;
}

Grid interpret_grid(const std::string &image_path)
{
    int width, height, channels;
    unsigned char *img = stbi_load(image_path.c_str(), &width, &height, &channels, 3);
    if (!img)
    {
        std::cerr << "Failed to load image: " << image_path << std::endl;
        return Grid{};
    }

    const int cell_h = height / Grid::height;
    const int cell_w = width / Grid::width;
    const double shape_ratio = 0.6;

    Grid grid;
    for (int r = 0; r < Grid::height; ++r)
    {
        for (int c = 0; c < Grid::width; ++c)
        {
            long sum_r = 0, sum_g = 0, sum_b = 0;
            int pixels = 0;

            int y0 = static_cast<int>(r * cell_h + (cell_h * (1.0 - shape_ratio) / 2));
            int y1 = static_cast<int>(y0 + cell_h * shape_ratio);
            int x0 = static_cast<int>(c * cell_w + (cell_w * (1.0 - shape_ratio) / 2));
            int x1 = static_cast<int>(x0 + cell_w * shape_ratio);

            for (int y = y0; y < y1; ++y)
            {
                for (int x = x0; x < x1; ++x)
                {
                    if (y >= height || x >= width)
                        continue;
                    int idx = (y * width + x) * 3;
                    sum_r += img[idx + 0];
                    sum_g += img[idx + 1];
                    sum_b += img[idx + 2];
                    ++pixels;
                }
            }

            if (pixels == 0)
            {
                grid.at(r, c) = Grid::Cell::Empty;
                continue;
            }

            int avg_r = static_cast<int>(sum_r / pixels);
            int avg_g = static_cast<int>(sum_g / pixels);
            int avg_b = static_cast<int>(sum_b / pixels);
            double h, s, v;
            rgb_to_hsv(avg_r, avg_g, avg_b, h, s, v);
            grid.at(r, c) = identify_shape_hsv(h, s, v);
        }
    }
    stbi_image_free(img);
    return grid;
}
