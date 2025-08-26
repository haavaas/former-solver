#pragma once
#include <array>
#include <iostream>
#include <vector>
#include <cstdint>

class Grid
{
public:
    static constexpr int width = 7;
    static constexpr int height = 9;

    template <typename T>
    using GridArray = std::array<std::array<T, width>, height>;
    using Coord = std::pair<uint8_t, uint8_t>;
    using Move = std::vector<Coord>;

    enum class Cell : uint8_t
    {
        Empty = 0,
        Blue = 1,
        Green = 2,
        Orange = 3,
        Pink = 4,
        Move = 5
    };

    Grid() : data_{} {}
    Grid(const GridArray<Cell> &data) : data_(data) {};
    Cell &at(int row, int col) { return data_[row][col]; }
    Cell at(int row, int col) const { return data_[row][col]; }
    const GridArray<Cell> &data() const { return data_; }

    std::vector<Move> get_moves() const noexcept;
    int count_moves() const noexcept;
    void play(const Move &move) noexcept;
    Move expand_move(const Coord &coord);
    void print_move(const Move &move) const;
    int count_occupied() const noexcept {
        int count = 0;
        for (const auto &row : data_) {
            for (const auto &cell : row) {
                if (cell != Cell::Empty) {
                    ++count;
                }
            }
        }
        return count;
    }

    bool operator==(const Grid &other) const { return data_ == other.data_; }

    friend std::ostream &operator<<(std::ostream &os, const Grid &grid);

private:
    GridArray<Cell> data_;
};