#include "Satellites.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    U16 num_sats = 0xFFFF;
    if (argc > 1) {
        num_sats = std::stoi(argv[1]);
    }

    std::cout << "[Satellite Worker] Booting up with " << num_sats << " satellites...\n";

    Satellite_Processor sat_proc;
    sat_proc.populate(num_sats);

    std::string opt;

    while(std::cin >> opt){
        if(opt == "exit") break;
        if(opt == "print"){
            int idx;
            std::cin >> idx;

            auto [x, y, z] = sat_proc.get_position(idx);

            std::cout << "Satellite [" << idx << "] [@" << sat_proc.get_elapsed_time().count() << "] : <" << x << ", " << y << ", " << z << "> [" << std::sqrt(x*x + y*y + z*z) << "]\n>> ";
        }
    }

    return 0;
}