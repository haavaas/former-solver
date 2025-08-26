#include <chrono>
#include "grid.hpp"
#include "grid_fetcher.hpp"
#include "solver.hpp"
#include <iostream>
#include "cli.hpp"
#include <ranges>

auto G = Grid::Cell::Green;
auto B = Grid::Cell::Blue;
auto R = Grid::Cell::Pink;
auto Y = Grid::Cell::Orange;


Grid::GridArray<Grid::Cell> example_grid_data = {{
    {{G, Y, Y, R, G, G, R}},
    {{B, R, B, R, R, R, Y}},
    {{G, G, B, Y, G, B, R}},
    {{R, B, R, R, Y, G, G}},
    {{Y, Y, G, B, Y, G, R}},
    {{Y, B, R, Y, Y, B, B}},
    {{R, R, Y, G, Y, G, B}},
    {{G, Y, B, Y, Y, Y, Y}},
    {{R, G, G, B, B, B, Y}},
}};


int main(int argc, char **argv)
{

    auto parsed = parse_cli(argc, argv);
    if (parsed.show_help)
    {
        print_help(argv[0]);
        return 0;
    }
    if (!parsed.ok)
    {
        std::cout << parsed.error << "\n";
        print_help(argv[0]);
        return parsed.exit_code ? parsed.exit_code : 1;
    }

    const auto &cfg = parsed.settings;
    std::cout << "Config:\n"
              << "  Search width: " << cfg.search_width << "\n"
              << "  Threads:      " << cfg.threads << "\n";

    Grid grid = fetch_grid();
    //Grid grid(example_grid_data);
    const std::size_t max_depth = Grid::width * Grid::height;
    
    
    auto t0 = std::chrono::steady_clock::now();
    BeamSolution beam_solution = solve_beam_parallel(grid, cfg.search_width, cfg.threads, max_depth);

    auto t1 = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = t1 - t0;
    
    std::cout << "Elapsed time: " << elapsed.count() << " seconds\n";
    std::cout << "Boards Analyzed: " << beam_solution.boards_analyzed << "\n";
    std::cout << "Bps: " << (beam_solution.boards_analyzed / elapsed.count()) << "\n";
    std::cout << "Hash map size: " << get_hash_map_size() << "\n";

    if (!beam_solution.solved)
    {
        std::cout << "No solution found within the given constraints.\n";
    }
    else
    {
        std::cout << "Solution found with " << beam_solution.moves.size() << " moves.\n";
    }
    std::cout << "Duplicates Dropped: " << beam_solution.duplicates_dropped << "\n";

    for (int i = 0; i < beam_solution.moves.size() - 1; ++i)
    {
        auto move = grid.expand_move(beam_solution.moves[i]);
        std::cout << "# possible moves: " << grid.count_moves() << "\n";
        grid.print_move(move);
        grid.play(move);
    }
    auto last_move = grid.get_moves();
    grid.print_move(last_move[0]);

    return 0;
}