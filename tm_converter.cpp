#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

#include "turing_machine.h"

static void print_usage(std::string error) {
    std::cerr << "ERROR: " << error << "\n"
              << "Usage: tm_interpreter <input_file> <output_file>\n";
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 3) print_usage("Bad number of arguments");

    std::string filename = argv[1];
    std::string outFilename = argv[2];

    FILE *f = fopen(filename.c_str(), "r");
    if (!f) {
        std::cerr << "ERROR: File " << filename << " does not exist\n";
        return 1;
    }
    TuringMachine tm = read_tm_from_file(f);

    //-----------------CONVERSION-----------------//
    tm.twoToOne();

    std::ofstream file(outFilename);
    file << tm;
    file.close();
}
