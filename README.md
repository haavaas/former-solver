Solves NRK Former for optimal solution. Aim it at a well cropped image of a game board
and get a solution.

Increased serach width can give a better solution, but the solver runs slower and requires more memory

Usage
  ./former_solver <image_path> [search_width] [threads]
  ./former_solver <image_path> -w <search_width> -t <threads>
  ./former_solver --help

Arguments:
  <image_path>           Mandatory. Path to the image file.
  [search_width]         Optional. Integer > 0. Defaults to 1000.
  [threads]              Optional. Integer > 0. Defaults to 4.

Options:
  -w, --width <int>      Search/beam width.
  -t, --threads <int>    Number of worker threads.
  -h, --help             Show this help and exit.

Examples:
  ./former_solver board.png
  ./former_solver board.png 5000 8
  ./former_solver board.png -w 5000 -t 8
  