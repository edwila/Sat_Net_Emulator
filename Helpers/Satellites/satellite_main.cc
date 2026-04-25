#include "Satellites.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    U16 num_sats = 0xFFFF;
    if (argc > 1) {
        num_sats = std::stoi(argv[1]);
    }

    std::cout << "[Satellite Worker] Opening shared memory with station...\n";

    int station_satellite_fd = shm_open("/station_satellite_communication", O_RDWR, 0600);

    unsigned long long len = sizeof(satellite_station_container);

    satellite_station_container* chunk1 = (satellite_station_container*)mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED_VALIDATE, station_satellite_fd, 0);

    std::cout << "[Satellite Worker] Shared memory chunk loaded successfully. Is initialized? " << std::boolalpha << chunk1->initialized << "\n";

    std::cout << "[Satellite Worker] Booting up with " << num_sats << " satellites...\n";

    Satellite_Processor sat_proc;
    sat_proc.populate(num_sats);

    std::cout << "[Satellite Worker] Spawning thread for request handling...\n";

    std::thread([chunk1, &sat_proc](){
        while(true){
            while(chunk1->req_sat_tail != chunk1->req_station_tail){
                // There is a request!
                auto sat_id = chunk1->requests[chunk1->req_sat_tail & 63];
                auto [x, y, z] = sat_proc.get_position(sat_id);
                chunk1->responses[chunk1->res_sat_tail & 63] = {sat_id, x, y, z};
                chunk1->res_sat_tail.fetch_add(1);
                chunk1->req_sat_tail.fetch_add(1);
            }

            std::this_thread::yield();
        }
    }).detach();

    std::cout << "[Satellite Worker] Buffer thread spawned!\n";

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

    return 0;
}