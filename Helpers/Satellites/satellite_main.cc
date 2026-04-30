#include "Satellites.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    U16 num_sats = 0xFFFF;
    if (argc > 1) {
        num_sats = std::stoi(argv[1]);
    }

    out("[Satellite Worker] Opening shared memory with station...");

    int global_rf_space_fd = shm_open("/global_rf_space", O_RDWR, 0666);
    // This shared memory will be for satellites communicating with the station (satellite requesting routing table, station providing routing table)
    int user_sat_rf_space_fd = shm_open("/user_sat_rf_space", O_RDWR, 0666);

    unsigned long long len = sizeof(shared_mem_container);
    unsigned long long user_sat_len = sizeof(user_sat_mem);

    int user_sat_truncate = ftruncate(user_sat_rf_space_fd, user_sat_len);

    if(user_sat_truncate == -1){
        out("[Satellite Worker] Failed to truncate fd [", user_sat_rf_space_fd, "]! Err: ", errno);

        return 1;
    }

    shared_mem_container* chunk1 = (shared_mem_container*)mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED, global_rf_space_fd, 0);
    user_sat_mem* chunk2 = (user_sat_mem*)mmap(nullptr, user_sat_len, PROT_READ | PROT_WRITE, MAP_SHARED, user_sat_rf_space_fd, 0);

    out("[Satellite Worker] Shared memory chunk loaded successfully.");

    out("[Satellite Worker] Booting up with ", num_sats, " satellites...");

    Satellite_Processor sat_proc(&chunk1->container, &chunk1->user_container);
    sat_proc.populate(num_sats);

    out("[Satellite Worker] Starting routing thread...");

    std::queue<packet>* sat_proc_q = sat_proc.get_queue();

    time_pq* latency = sat_proc.get_latency();

    std::thread([chunk1, chunk2, &sat_proc, sat_proc_q, latency](){
        while(sat_proc.is_alive()){
            while(chunk1->read_tail != chunk1->write_tail){
                // There are packets needing to be routed
                packet read_packet = chunk1->packets[chunk1->read_tail & 63];
                chunk1->read_tail.fetch_add(1);

                sat_proc.process_packet(chunk1, chunk2, read_packet);
            }

            U32 current_time = sat_proc.get_elapsed_time();
            while(!latency->empty()) {
                auto top = latency->top();
                
                if(top.first > current_time) {
                    break; 
                }

                sat_proc_q->push(top.second);
                latency->pop();
            }

            while(!sat_proc_q->empty()){
                sat_proc.process_packet(chunk1, chunk2, sat_proc_q->front());
                sat_proc_q->pop();
            }

            std::this_thread::yield();
        }
    }).detach();

    out("[Satellite Worker] Ready.");

    std::string opt;

    while(std::cin >> opt){
        if(opt == "exit") break;
        if(opt == "print"){
            int idx;
            std::cin >> idx;

            auto [x, y, z] = sat_proc.get_position(idx);

            out("Satellite [", idx, "] [@", sat_proc.get_elapsed_time(), "] : <", x, ", ", y, ", ", z, "> [", std::sqrt(x*x + y*y + z*z), "]");
        }

    }

    sat_proc.kill();

    return 0;
}