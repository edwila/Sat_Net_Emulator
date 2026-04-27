#include "Satellites.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    U16 num_sats = 0xFFFF;
    if (argc > 1) {
        num_sats = std::stoi(argv[1]);
    }

    std::cout << "[Satellite Worker] Opening shared memory with station...\n";

    int global_rf_space_fd = shm_open("/global_rf_space", O_RDWR, 0600);
    // This shared memory will be for satellites communicating with the station (satellite requesting routing table, station providing routing table)

    unsigned long long len = sizeof(shared_mem_container);

    int truncate_result = ftruncate(global_rf_space_fd, len);

    if(truncate_result == -1){
        std::cout << "[Satellite Worker] Failed to truncate fd [" << global_rf_space_fd << "]! Err: " << errno << "\n";

        return 1;
    }

    shared_mem_container* chunk1 = (shared_mem_container*)mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED, global_rf_space_fd, 0);

    std::cout << "[Satellite Worker] Shared memory chunk loaded successfully. Is initialized? " << std::boolalpha << chunk1->initialized << "\n";

    std::cout << "[Satellite Worker] Booting up with " << num_sats << " satellites...\n";

    Satellite_Processor sat_proc(&chunk1->container);
    sat_proc.populate(num_sats);

    std::cout << "[Satellite Worker] Ready.\n";

    std::string opt;

    std::cout << ">> ";

    while(std::cin >> opt){
        if(opt == "exit") break;
        if(opt == "print"){
            int idx;
            std::cin >> idx;

            auto [x, y, z] = sat_proc.get_position(idx);

            std::cout << "Satellite [" << idx << "] [@" << sat_proc.get_elapsed_time().count() << "] : <" << x << ", " << y << ", " << z << "> [" << std::sqrt(x*x + y*y + z*z) << "]\n>> ";
        }
    }

    sat_proc.kill();

    return 0;
}