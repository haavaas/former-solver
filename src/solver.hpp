#pragma once
#include "grid.hpp"
#include <vector>

struct BeamSolution
{
    bool solved = false;
    std::vector<Grid::Coord> moves;
    int boards_analyzed = 0;
    int hash_size = 0;
    int duplicates_dropped = 0;
};

BeamSolution solve_beam_parallel(const Grid &start,
                                 std::size_t beam_width = 500,
                                 std::size_t max_threads = 0,
                                 std::size_t max_depth = Grid::width * Grid::height);

size_t get_hash_map_size(void);