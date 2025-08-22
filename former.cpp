#include <stdio.h>
#include <stdint-gcc.h>
#include <array>
#include <iostream>
#include <vector>
#include <algorithm>
#include <sys/resource.h>
#include <stack>
#include <chrono>
#include "board_annotate.hpp"

static constexpr char toLetter(uint8_t sq) {
    return "-RGBY"[sq];
}

typedef uint8_t Square;
typedef std::pair<int,int> Coord;
typedef std::vector<Coord> Move;

std::ostream& operator<<(std::ostream& os, const Coord& co) {
    os << "(" << co.first << "," << co.second << ")";
    return os;
}

class Vertical : public std::array<Square, VSIZE> {
public:
    Vertical(const std::array<Square, VSIZE>& ref) : std::array<Square, VSIZE>(ref) {}
    void fall(){
        for(auto it = this->rbegin(); it != this->rend();) {
            if(*it == 0){
                bool all_zero = true;
                for(auto x = it; x != this->rend(); x++){
                    if(*x){
                        all_zero = false;
                        break;
                    }
                }
                if(all_zero){
                    break;
                }
                for(auto it2 = it; it2 != (this->rend() - 1); it2++) {
                    *it2 = *(it2 + 1);
                }
                *this->begin() = 0;
            } else {
                it++;
            }
        }
    }
};

class Board : public std::array<Vertical, HSIZE>{
public:
    Board(const std::array<Vertical, HSIZE>& ref) : std::array<Vertical, HSIZE>(ref) {}
    void fall() {
        for(auto& elem : *this){
            elem.fall();
        }
    }
    Square at(const Coord co) {
#if 0 // Bounds checking
        return this->std::array<Vertical, HSIZE>::at(co.first).at(co.second);
#else // YOLO
        return (*this)[co.first][co.second];
#endif
    }
};

std::ostream& operator<<(std::ostream& os, const Board& arr) {
    for (int y = 0; y < VSIZE; y++) {
        for (int x = 0; x < HSIZE; x++) {
            os << toLetter(arr[x][y]) << ' ';
        }
        os << '\n';
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const std::stack<Coord, Move>& stack) {
    std::stack<Coord, Move> tmp = stack;
    while (tmp.size() > 0) {
        os << tmp.top() << ",";
        tmp.pop();
    }
    os << std::endl;
    return os;
}

uint64_t calculated_states = 0;
class Game{
public:
    Board board;
    std::vector<Move> moves;
    Coord origin;
    Game() = delete;
    Game(const Board& brd) : board(brd) {
        calculated_states++;
        moves.reserve(40);
    };
    Game(const Board& old, const Move& move) : board(old){
        std::array<bool, HSIZE> verts_updated = {false};
        origin = move[0];
        calculated_states++;
        moves.reserve(40);
        for(Coord co : move) {
            board[co.first][co.second] = 0;
            verts_updated[co.first] = true;
        }
        for(int i = 0; i < verts_updated.size(); i++){
            if(verts_updated[i]){
                board[i].fall();
            }
        }
    }

    bool has_co_in_moves(Coord co){
        for(const Move& mv : moves){
            if(std::find(mv.begin(), mv.end(), co) != mv.end()){
                return true;
            }
        }
        return false;
    }

    void expand_move(Move& mv, const Coord co, const Square sq){
        std::stack<Coord, Move> to_visit;
        bool scratch[HSIZE][VSIZE] = {false};

        to_visit.push(co);

        while (!to_visit.empty()) {
            Coord current = to_visit.top();
            to_visit.pop();
            scratch[current.first][current.second] = true;
            mv.push_back(current);

            Coord up(current.first, current.second - 1);
            Coord down(current.first, current.second + 1);
            Coord left(current.first - 1, current.second);
            Coord right(current.first + 1, current.second);
            if(up.second >= 0 && !scratch[up.first][up.second] && board.at(up) == sq){
                to_visit.push(up);
            }
            if(down.second < VSIZE && !scratch[down.first][down.second] && board.at(down) == sq){
                to_visit.push(down);
            }
            if(left.first >= 0 && !scratch[left.first][left.second] && board.at(left) == sq){
                to_visit.push(left);
            }
            if(right.first < HSIZE && !scratch[right.first][right.second] && board.at(right) == sq){
                to_visit.push(right);
            }
        }
    }

    // void expand_move(Move& mv, const Coord co, const Square sq){
    //     mv.push_back(co);
    //     Coord up(co.first, co.second - 1);
    //     Coord down(co.first, co.second + 1);
    //     Coord left(co.first - 1, co.second);
    //     Coord right(co.first + 1, co.second);
    //     if(up.second >= 0 && board.at(up) == sq && std::find(mv.begin(), mv.end(), up) == mv.end()){
    //         expand_move(mv, up, sq);
    //     }
    //     if(down.second < VSIZE && board.at(down) == sq && std::find(mv.begin(), mv.end(), down) == mv.end()){
    //         expand_move(mv, down, sq);
    //     }
    //     if(left.first >= 0 && board.at(left) == sq && std::find(mv.begin(), mv.end(), left) == mv.end()){
    //         expand_move(mv, left, sq);
    //     }
    //     if(right.first < HSIZE && board.at(right) == sq && std::find(mv.begin(), mv.end(), right) == mv.end()){
    //         expand_move(mv, right, sq);
    //     }
    // }

    // void expand_move(Move& mv, std::array<std::array<bool, VSIZE>, HSIZE>& scratch,  const Coord co, const Square sq){
    //     scratch[co.first][co.second] = true;
    //     mv.push_back(co);
    //     Coord up(co.first, co.second - 1);
    //     Coord down(co.first, co.second + 1);
    //     Coord left(co.first - 1, co.second);
    //     Coord right(co.first + 1, co.second);
    //     if(up.second >= 0 && board.at(up) == sq && !scratch[up.first][up.second]){
    //         expand_move(mv, scratch, up, sq);
    //     }
    //     if(down.second < VSIZE && board.at(down) == sq && !scratch[down.first][down.second]){
    //         expand_move(mv, scratch, down, sq);
    //     }
    //     if(left.first >= 0 && board.at(left) == sq && !scratch[left.first][left.second]){
    //         expand_move(mv, scratch, left, sq);
    //     }
    //     if(right.first < HSIZE && board.at(right) == sq && !scratch[right.first][right.second]){
    //         expand_move(mv, scratch, right, sq);
    //     }
    // }

    int calculate_moves(){
        //std::array<std::array<bool, VSIZE>, HSIZE> scratch = {false};
        for (int y = 0; y < VSIZE; y++) {
            for (int x = 0; x < HSIZE; x++) {
                Square sq = board[x][y];
                Coord co(x,y);
                if(sq && !has_co_in_moves(co)){
                    Move mv;
                    mv.reserve(20);
                    expand_move(mv, co, sq);
                    moves.push_back(mv);
                }
            }
        }
        return moves.size();
    }

};

bool compare_games(const std::pair<int, Game>& lh, const std::pair<int, Game>& rh){
    return lh.first < rh.first;
}

bool done = false;
std::stack<Coord, Move> solution;

class Search {
public:
    bool done = false;
    std::stack<Coord, Move> solution;
    void search(Game game, int depth, std::size_t width){
        std::vector<std::pair<int, Game>> games;
        if(depth-- == 0 || done){
            return;
        }
        for(int i = 0; i < game.moves.size(); i++){
            Game new_game(game.board, game.moves[i]);
            games.push_back({new_game.calculate_moves(), new_game});
        }
        std::stable_sort(games.begin(), games.end(), compare_games);
        if(width > 5){
            width--;
        }
        for(int i = 0; i < std::min(width, games.size()); i++){
            auto& pair = games[i];
            if(pair.first <= 2){
                done = true;
                std::cout << pair.second.board << depth << std::endl << std::endl;
            } else if (pair.first < (depth + 3)*3.6) {
                search(pair.second, depth, width);
            }
            if (done) {
                solution.push(pair.second.origin);
                return;
            }
        }
        return;
    }
};

// void mt_search(Game game, int depth, std::size_t width, int threads){
//     std::vector<std::pair<int, Game>> games;
//     if(depth-- == 0 || done){
//         return;
//     }
//     for(int i = 0; i < game.moves.size(); i++){
//         Game new_game(game.board, game.moves[i]);
//         games.push_back({new_game.calculate_moves(), new_game});
//     }
//     std::stable_sort(games.begin(), games.end(), compare_games);
//     if(width > 5){
//         width--;
//     }
//     for(int i = 0; i < std::min(width, games.size()); i++){
//         auto& pair = games[i];
//         if(pair.first <= 2){
//             done = true;
//             std::cout << pair.second.board << depth << std::endl << std::endl;
//         } else if (pair.first < (depth + 3)*4) {
//             search(pair.second, depth, width);
//         }
//         if (done) {
//             solution.push(pair.second.origin);
//             return;
//         }
//     }
//     return;
// }

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_image>\n";
        return 1;
    }
    std::string input_image = argv[1];
    auto newBoard = Annotator::analyzeBoard(input_image, Annotator::Params());
    for (auto x : newBoard) {
        for (auto y : x) {
            std::cout << toLetter(y) << ' ';
        }
        std::cout << '\n';
    }
    // Board board = {{ \ 
    //     Vertical({Y, B, G, G, R, B, Y, G, G}), \
    //     Vertical({B, B, G, G, Y, B, G, G, B}), \
    //     Vertical({G, G, R, R, Y, B, G, B, G}), \
    //     Vertical({Y, B, R, R, G, B, G, Y, B}), \
    //     Vertical({G, B, R, B, G, G, B, B, Y}), \
    //     Vertical({B, G, R, B, Y, B, R, Y, G}), \
    //     Vertical({R, G, R, R, Y, R, Y, B, R}), \
    // }}; // 15/8 beste 14
    // Board board = {{ \ 
    //     Vertical({G, B, G, R, Y, Y, R, G, R}), \
    //     Vertical({Y, R, G, B, Y, B, R, Y, G}), \
    //     Vertical({Y, B, B, R, G, R, Y, B, G}), \
    //     Vertical({R, R, Y, R, B, Y, G, Y, B}), \
    //     Vertical({G, R, G, Y, Y, Y, Y, Y, B}), \
    //     Vertical({G, R, B, G, G, B, G, Y, B}), \
    //     Vertical({R, Y, R, G, R, B, B, Y, Y}), \
    // }}; // 18/8 beste 12
    // Board board = {{ \
    //     Vertical({R, G, Y, Y, B, G, B, R, G}), \
    //     Vertical({G, B, R, R, R, Y, Y, R, G}), \
    //     Vertical({B, G, Y, R, Y, R, Y, R, B}), \
    //     Vertical({R, G, Y, B, R, G, R, B, G}), \
    //     Vertical({G, G, B, B, Y, G, G, G, G}), \
    //     Vertical({B, G, Y, R, R, B, G, Y, Y}), \
    //     Vertical({R, B, G, Y, R, G, G, R, B}), \
    // }};  // 20/8 beste 13
    Board board = {{ \ 
        Vertical({R, R, B, B, G, G, R, Y, G}), \
        Vertical({G, B, G, R, Y, G, R, G, Y}), \
        Vertical({G, R, G, Y, R, B, G, R, R}), \
        Vertical({R, Y, G, R, R, Y, B, B, B}), \
        Vertical({B, Y, Y, R, G, B, G, G, R}), \
        Vertical({B, R, G, Y, Y, R, G, G, G}), \
        Vertical({G, B, Y, R, B, R, R, Y, R}), \
    }}; // 21/8 beste 14
    Game game(board);
    std::cout << "Hello" << std::endl;
    std::cout << board << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();
    std::cout << game.calculate_moves() << " possible moves in initial board" << std::endl;
    Search thread;
    thread.search(game, 12, 12);
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration_s = end_time - start_time;
    std::cout << "Search took " << duration_s.count() << " s, and generated " << calculated_states << " board states\n";
    std::cout << "Solution was: " << solution;
}
