Solves NRK Former for optimal solution. Uses magic and wizardry to generate todays board without internet access and gets a solution.

Increased search width can give a better solution, but the solver runs slower and requires more memory. Recommended widths are in the range [200-100000]

Build requirements:
g++ that supports c++20
ninja-build
libssl-dev
libcurl4-openssl-dev

Tested with ubuntu in WSL2 and ubuntu 24.04 native.

Build:
ninja

Usage:
  ./former_solver [search_width] [threads]
  ./former_solver -w <search_width> -t <threads>
  ./former_solver --help

Arguments:
  [search_width]         Optional. Integer > 0. Defaults to 1000.
  [threads]              Optional. Integer > 0. Defaults to 4.

Options:
  -w, --width <int>      Search/beam width.
  -t, --threads <int>    Number of worker threads.
  -h, --help             Show this help and exit.

Examples:
  ./former_solver
  ./former_solver 5000 8
  ./former_solver -w 5000 -t 8
