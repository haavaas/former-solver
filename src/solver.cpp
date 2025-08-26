#include <future>
#include "grid.hpp"
#include <algorithm>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <limits>
#include <cstring>
#include "solver.hpp"

struct CompressedGrid
{
    std::array<uint64_t, 3> data;

    CompressedGrid(const Grid &g)
    {
        std::memset(data.data(), 0, sizeof(data));
        uint8_t bit = 0;
        for (uint8_t r = 0; r < Grid::height; ++r)
        {
            for (uint8_t c = 0; c < Grid::width; ++c)
            {
                uint64_t val = static_cast<uint64_t>(g.at(r, c)) & 0x7;
                uint8_t idx = bit / 64;
                uint8_t offset = bit % 64;
                data[idx] |= (val << offset);
                bit += 3;
            }
        }
    }

    bool operator==(const CompressedGrid &other) const
    {
        return data == other.data;
    }
};

struct CompressedGridHasher
{
    std::size_t operator()(const CompressedGrid &cg) const noexcept
    {
        std::size_t h = 0;
        for (auto x : cg.data)
        {
            h += x;
        }
        return h;
    }
};

uint8_t get_small_hash(size_t big_hash) {
    uint8_t small_hash = 0;
    for (uint8_t i = 0; i < 8; ++i) {
        small_hash ^= (big_hash >> (i * 8)) & 0xFF;
    }
    return small_hash;
}

struct Node
{
    Grid grid;
    std::vector<Grid::Coord> path;
    uint8_t cost = 0;

    bool operator<(const Node &other) const
    {
        return cost < other.cost;
    }
};

int cost (Grid g) {
    return g.count_moves();
}

static constexpr size_t num_shards = 256;
std::mutex known_boards_mutex[num_shards];
std::unordered_map<CompressedGrid, uint8_t, CompressedGridHasher> known_boards[num_shards];
size_t get_hash_map_size(void) {
    size_t total_size = 0;
    for (const auto &set : known_boards) {
        total_size += set.size();
    }
    return total_size;
}


BeamSolution solve_beam(Grid start,
                        std::vector<Grid::Move> initial_moves,
                        std::size_t beam_width,
                        std::size_t max_depth)
{
    BeamSolution result;
    result.boards_analyzed = 1;

    std::multiset<Node> beam;
    std::multiset<Node> next_beam;
    
    if (initial_moves.empty())
    {
        initial_moves = start.get_moves();
    }

    for (const auto &move : initial_moves)
    {
        Grid g = start;
        g.play(move);
        Node node{std::move(g), {move[0]}, 0};
        beam.insert(std::move(node));
    }

    for (int depth = 0; depth < static_cast<int>(max_depth); ++depth)
    {
        for (const auto &node : beam)
        {
            for (const auto &mv : node.grid.get_moves())
            {
                Grid child = node.grid;
                child.play(mv);
                ++result.boards_analyzed;

                int h = cost(child);
                if (child.count_moves() < 3)
                {
                    result.solved = true;
                    result.moves = node.path;
                    result.moves.push_back(mv[0]);
                    for (const auto &m : child.get_moves())
                    {
                        result.moves.push_back(m[0]);
                    }
                    return result;
                }
                
                
                auto cg = CompressedGrid(child);
                uint8_t small_hash = get_small_hash(CompressedGridHasher()(cg)) % num_shards;
                auto& bucket = known_boards[small_hash];
                auto& mutex = known_boards_mutex[small_hash];
                bool drop = true;
                
                mutex.lock();
                auto hit = bucket.find(cg);
                if (hit == bucket.end() || hit->second > depth) {
                    bucket[cg] = depth;
                    drop = false;
                }
                mutex.unlock();

                if (drop) {
                    result.duplicates_dropped++;
                    continue;
                }

                Node cand;
                cand.grid = std::move(child);
                cand.path = node.path;
                cand.path.push_back(mv[0]);
                cand.cost = h;

                if (next_beam.size() < beam_width)
                {
                    next_beam.insert(std::move(cand));
                }
                else
                {
                    auto worst_it = std::prev(next_beam.end());
                    if (cand < *worst_it)
                    {
                        next_beam.erase(worst_it);
                        next_beam.insert(std::move(cand));
                    }
                }
            }
        }

        if (next_beam.empty())
            break;
        next_beam.swap(beam);
        next_beam.clear();
    }

    if (!beam.empty())
    {
        const Node &best = *beam.begin();
        result.solved = false;
        result.moves = best.path;
    }

    return result;
}

BeamSolution solve_beam_parallel(const Grid &start,
                                 std::size_t beam_width,
                                 std::size_t max_threads,
                                 std::size_t max_depth)
{
    if (max_threads == 0)
        max_threads = std::thread::hardware_concurrency();
    
    if (max_threads == 0)
        max_threads = 2;
    
    auto initial_moves = start.get_moves();
    if (initial_moves.empty())
        return {};

    std::size_t actual_threads = std::min(max_threads, initial_moves.size());
    std::cout << "Using " << actual_threads << " threads for parallel beam search.\n";

    std::vector<std::vector<Grid::Move>> move_groups(actual_threads);
    for (std::size_t i = 0; i < initial_moves.size(); ++i)
    {
        move_groups[i % actual_threads].push_back(initial_moves[i]);
    }

    std::vector<std::future<BeamSolution>> futures;
    for (std::size_t t = 0; t < actual_threads; ++t)
    {
        futures.push_back(std::async(std::launch::async, [&, t]
                                     { return solve_beam(start, move_groups[t], beam_width, max_depth); }));
    }

    BeamSolution best;
    int idx = 0;
    for (auto &fut : futures)
    {
        BeamSolution solution = fut.get();
        best.boards_analyzed += solution.boards_analyzed;
        best.duplicates_dropped += solution.duplicates_dropped;
        idx++;
        std::cout << "Thread " << idx << ": Boards Analyzed = " << solution.boards_analyzed
                  << ", Solved = " << (solution.solved ? "Yes" : "No") << ", Moves: " << solution.moves.size()
                  << ", Duplicates Dropped: " << solution.duplicates_dropped << "\n";

        if (!best.solved && solution.solved)
        {
            best = std::move(solution);
        }
        else if (best.solved && solution.solved && solution.moves.size() < best.moves.size())
        {
            best = std::move(solution);
        }
        else if (!best.solved && solution.moves.size() < best.moves.size())
        {
            best = std::move(solution);
        }
    }

    return best;
}


