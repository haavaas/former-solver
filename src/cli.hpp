#pragma once

#include <string>
#include <iostream>

struct SolverSettings
{
    int search_width = 1000;
    int threads = 4;
};

struct ParseResult
{
    bool ok = false;
    bool show_help = false;
    SolverSettings settings{};
    std::string error;
    int exit_code = 1;
};

ParseResult parse_cli(int argc, char **argv);

void print_help(const char *prog);
