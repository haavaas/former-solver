#include "grid.hpp"
#include <stack>

std::vector<Grid::Move> Grid::get_moves() const noexcept {
    std::vector<Move> moves;
    moves.reserve(64);
    auto grid_copy = data_;
    std::vector<Coord> vec;
    vec.reserve(16);
    std::stack<Coord, std::vector<Coord>> discovered_neighbours(std::move(vec));
    

    for (uint8_t r = 0; r < height; ++r) {
        for (uint8_t c = 0; c < width; ++c) {
            if (grid_copy[r][c] == Cell::Empty)
                continue;
                
            Move move;
            move.reserve(16);
            discovered_neighbours.emplace(r, c);
            Cell cell = data_[r][c];

            while (!discovered_neighbours.empty()) {
                auto [cr, cc] = discovered_neighbours.top();
                discovered_neighbours.pop();
                move.emplace_back(cr, cc);
                grid_copy[cr][cc] = Cell::Empty;

                // Right
                if (cc + 1 < width && grid_copy[cr][cc + 1] == cell) {
                    discovered_neighbours.emplace(cr, cc + 1);
                }
                
                // Down
                if (cr + 1 < height && grid_copy[cr + 1][cc] == cell) {
                    discovered_neighbours.emplace(cr + 1, cc);
                }

                // Left
                if (cc > 0 && grid_copy[cr][cc - 1] == cell) {
                    discovered_neighbours.emplace(cr, cc - 1);
                }

                // Up
                if (cr > 0 && grid_copy[cr - 1][cc] == cell) {
                    discovered_neighbours.emplace(cr - 1, cc);
                }
            }
            moves.push_back(std::move(move));
        }
    }
    return moves;
}

int Grid::count_moves() const noexcept {
    int move_count = 0;
    auto grid_copy = data_;
    std::stack<Coord, std::vector<Coord>> discovered_neighbours;

    for (uint8_t r = 0; r < height; ++r) {
        for (uint8_t c = 0; c < width; ++c) {
            if (grid_copy[r][c] == Cell::Empty)
                continue;
            
            move_count++;
            discovered_neighbours.emplace(r, c);
            Cell cell = grid_copy[r][c];

            while (!discovered_neighbours.empty()) {
                auto [cr, cc] = discovered_neighbours.top();
                discovered_neighbours.pop();
                grid_copy[cr][cc] = Cell::Empty;

                // Right
                if (cc + 1 < width && grid_copy[cr][cc + 1] == cell) {
                    discovered_neighbours.emplace(cr, cc + 1);
                }
                
                // Down
                if (cr + 1 < height && grid_copy[cr + 1][cc] == cell) {
                    discovered_neighbours.emplace(cr + 1, cc);
                }

                // Left
                if (cc > 0 && grid_copy[cr][cc - 1] == cell) {
                    discovered_neighbours.emplace(cr, cc - 1);
                }

                // Up
                if (cr > 0 && grid_copy[cr - 1][cc] == cell) {
                    discovered_neighbours.emplace(cr - 1, cc);
                }
            }
        }
    }
    return move_count;
}

Grid::Move Grid::expand_move(const Coord &coord) {
    Move move{};
    std::vector<Coord> vec;
    vec.reserve(16);
    std::stack<Coord, std::vector<Coord>> discovered_neighbours(std::move(vec));
    GridArray<bool> visited{};
    discovered_neighbours.push(coord);
    Cell cell = data_[coord.first][coord.second];

    while (!discovered_neighbours.empty()) {
        auto [r, c] = discovered_neighbours.top();
        discovered_neighbours.pop();
        if (data_[r][c] != cell) {
            continue;
        }

        move.emplace_back(r, c);
        visited[r][c] = true;

        // Right
        if (c + 1 < width && !visited[r][c + 1]) {
            discovered_neighbours.emplace(r, c + 1);
        }

        // Down
        if (r + 1 < height && !visited[r + 1][c]) {
            discovered_neighbours.emplace(r + 1, c);
        }

        // Left
        if (c > 0 && !visited[r][c - 1]) {
            discovered_neighbours.emplace(r, c - 1);
        }

        // Up
        if (r > 0 && !visited[r - 1][c]) {
            discovered_neighbours.emplace(r - 1, c);
        }
    }

    return move;
}

void Grid::play(const Move& move) noexcept{
    for (auto [r, c] : move) {
        data_[r][c] = Cell::Empty;
    }

    for (int c = 0; c < width; ++c) {
        int write = height - 1;
        for (int r = height - 1; r >= 0; --r) {
            if (data_[r][c] != Cell::Empty) {
                if (write != r) {
                    data_[write][c] = data_[r][c];
                }
                --write;
            }
        }

        for (; write >= 0; --write) {
            data_[write][c] = Cell::Empty;
        }
    }
}

void Grid::print_move(const Move& move) const {
    Grid G = *this;
    for (const auto& [r, c] : move) {
        G.at(r, c) = Cell::Move;
    }
    std::cout << G << "\n";
}

std::ostream& operator<<(std::ostream& os, const Grid& grid) {
    for (const auto& row : grid.data_) {
        for (Grid::Cell val : row) {
            switch (val) {
                case Grid::Cell::Blue:   os << "B "; break;
                case Grid::Cell::Green:  os << "G "; break;
                case Grid::Cell::Orange: os << "O "; break;
                case Grid::Cell::Pink:   os << "P "; break;
                case Grid::Cell::Empty:  os << "- "; break;
                case Grid::Cell::Move:   os << "x "; break;
                default:                 os << "? "; break;
            }
        }
        os << "\n";
    }
    return os;
}
