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
    Station-Satellite communication:
    The satellite worker and the station will have two shared buffers
    [BUFFER 1 - REQUEST BUFFER]: This buffer will contain routing table requests from satellites (i.e., satellite 3 requests its routing table, ground station checks current timestamp, then gives it its routing table).
    [BUFFER 2 - RESPONSE BUFFER]: This buffer will contain the actual routing table. It will also contain a satellite_id so that the satellite that requested it can get it.
    */

    /*
    We want to emulate this communication scheme:
        User -> Satellite -> ... Satellite ... -> Ground Station/User

    Right now, our communication scheme is facing in this direction:
        User -> Ground Station -> ... Satellite ... -> Ground Station
    
    We need to get rid of the first Ground Station call. We can do this by:
        User:
            - Calculates optimal satellite to connect to (using Beam Planner)
            - Writes packet to the "global RF" (simulates writing to the satellite/sending it a signal) (shared memory between satellite driver and user driver)
        
        Satellite:
            - Reads from global RF buffer to pick up packets
            - Simulates latency, then routes them

    So where does the station come in play?
    The answer to that is whenever the satellites need it!
    
    The station will be responsible for generating the routing tables that the satellites will retrieve (from the shared memory).
    
    The station generates routing tables in advance, writes the current timestamped one to memory, and continues on with its life.
    Satellites retrieve their routing table entry, then go on with their lives.
    */

    int global_rf_space_fd = shm_open("/global_rf_space", O_CREAT | O_RDWR, 0600);
    // This shared memory will be for satellites communicating with the station (satellite requesting routing table, station providing routing table)

    unsigned long long len = sizeof(shared_mem_container);

    int truncate_result = ftruncate(global_rf_space_fd, len);

    if(truncate_result == -1){
        std::cout << "[Station] Failed to truncate fd [" << global_rf_space_fd << "]! Err: " << errno << "\n";

        return 1;
    }


    // Ground station only ever truly communicates with the satellites

    // But our satellites will exist in shared memory (so that user can run beam planner on it)

    shared_mem_container* chunk1 = (shared_mem_container*)mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED, global_rf_space_fd, 0);

    chunk1->initialized = true;

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
            close(global_rf_space_fd);
            // unlink
            shm_unlink("/station_satellite_communication");

            std::cout << "[Station] Shared memory cleaned up! Graceful exit.\n";

            break;
        } else if(opt == "request"){
            U16 sat_id;
            std::cin >> sat_id;

            std::cout << "[Station] Requesting satellite " << sat_id << " information...\n";

            float X = chunk1->container.positions.X[sat_id], Y = chunk1->container.positions.Y[sat_id], Z = chunk1->container.positions.Z[sat_id];
            
            std::cout << "[Station] Satellite [" << sat_id << "]: <" << X << ", " << Y << ", " << Z << "> [" << std::sqrt(X*X + Y*Y + Z*Z) << "]\n";
        }

        std::cout << ">> ";
    }
    
    return 0;
};