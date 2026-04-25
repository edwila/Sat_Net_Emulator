#ifndef STATION_ACCESS
#define STATION_ACCESS
#endif

#include "../Dependencies/consts.hpp"
#include <iostream>
#include <string>
#include <cmath>

int main(){
    U16 num_sats, num_users;    

    std::cout << "[Station] Enter number of satellites: ";
    std::cin >> num_sats;

    std::cout << "[Station] Enter number of users: ";
    std::cin >> num_users;

    // Reserve shared memory here, before launching workers
    // We want 2 blocks total of reserved memory

    /*
    FIRST CHUNK:
    Station-Satellite communication:
    The satellite worker and the station will have two shared buffers
    [BUFFER 1 - REQUEST BUFFER] : This buffer will contain satellite IDs (array of raw U16). Station will write to this, satellite will read from this. It will contain the satellites that station needs.
    [BUFFER 2 - RESPONSE BUFFER] : This buffer will contain the response struct. Response structs (defined in consts.hpp) consist of sat_id (U16) and 3x float (X, Y, Z).
    */

    int station_satellite_fd = shm_open("/station_satellite_communication", O_CREAT | O_RDWR, 0600);

    unsigned long long len = sizeof(satellite_station_container);

    int truncate_result = ftruncate(station_satellite_fd, len);

    if(truncate_result == -1){
        std::cout << "[Station] Failed to truncate fd [" << station_satellite_fd << "]! Err: " << errno << "\n";

        return 1;
    }

    satellite_station_container* chunk1 = (satellite_station_container*)mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED_VALIDATE, station_satellite_fd, 0);

    chunk1->initialized = true;

    /*
    SECOND CHUNK:
    Station-User communication:
    The user worker and the station worker will have two shared buffers
    [BUFFER 1 - REQUEST BUFFER] : This buffer will contain satellite IDs (array of raw U16). User will write to this, station will read from this and copy them to its [BUFFER 1] within its communication to satellite worker.
    [BUFFER 2 - RESPONSE BUFFER] : This buffer will contain the response structs (as defined in consts.hpp). These will be the responses that the station will receive from the satellite worker.
    */

    std::cout << "[Station] Launching workers...\n";

    system(("cmd.exe /c start wsl ./satellite_worker " + std::to_string(num_sats)).c_str());
    system(("cmd.exe /c start wsl ./user_worker " + std::to_string(num_users)).c_str());

    std::cout << "[Station] Workers launched!\n";

    std::string opt;

    std::cout << ">> ";

    while(std::cin >> opt){
        if(opt == "exit"){
            // unmap
            munmap((void*)chunk1, len);
            // close the fd
            close(station_satellite_fd);
            // unlink
            shm_unlink("/station_satellite_communication");

            std::cout << "[Station] Shared memory cleaned up! Graceful exit.\n";

            break;
        } else if(opt == "request"){
            U16 sat_id;
            std::cin >> sat_id;

            std::cout << "[Station] Requesting satellite " << sat_id << " information...\n";
            chunk1->requests[chunk1->req_station_tail & 63] = sat_id;
            chunk1->req_station_tail.fetch_add(1);
            while(chunk1->res_station_tail == chunk1->res_sat_tail){}
            while(chunk1->res_station_tail != chunk1->res_sat_tail){
                auto storage = chunk1->responses[chunk1->res_station_tail & 63];
                if(storage.satellite_id == sat_id){
                    // Received information!
                    std::cout << "[Station] Satellite [" << storage.satellite_id << "]: <" << storage.X << ", " << storage.Y << ", " << storage.Z << "> [" << std::sqrt(storage.X*storage.X + storage.Y*storage.Y + storage.Z*storage.Z) << "]\n";
                }
                chunk1->res_station_tail.fetch_add(1);
            }
        }

        std::cout << ">> ";
    }
    
    return 0;
};