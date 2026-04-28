#include "Users.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    U16 num_users = 0xFFFF; 
    if (argc > 1) {
        num_users = std::stoi(argv[1]);
    }

    out("[User Worker] Booting up with ", num_users, " users...");

    int global_rf_space_fd = shm_open("/global_rf_space", O_RDWR, 0600);
    // This shared memory will be for satellites communicating with the station (satellite requesting routing table, station providing routing table)
    int user_sat_rf_space_fd = shm_open("/user_sat_rf_space", O_RDWR, 0600);

    unsigned long long len = sizeof(shared_mem_container);
    unsigned long long user_sat_len = sizeof(user_sat_mem);

    int truncate_result = ftruncate(global_rf_space_fd, len);
    int user_sat_truncate = ftruncate(user_sat_rf_space_fd, user_sat_len);

    if(truncate_result == -1){
        out("[Satellite Worker] Failed to truncate fd [", global_rf_space_fd, "]! Err: ", errno);

        return 1;
    }

    if(user_sat_truncate == -1){
        out("[Satellite Worker] Failed to truncate fd [", user_sat_rf_space_fd, "]! Err: ", errno);

        return 1;
    }

    shared_mem_container* chunk1 = (shared_mem_container*)mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED, global_rf_space_fd, 0);
    user_sat_mem* chunk2 = (user_sat_mem*)mmap(nullptr, user_sat_len, PROT_READ | PROT_WRITE, MAP_SHARED, user_sat_rf_space_fd, 0);

    out("[User Worker] Shared memory allocated successfully.");

    User_Processor user_proc(&chunk1->container);
    user_proc.populate(num_users);

    out("[User Worker] Spawning thread to handle messages...");

    std::thread([chunk2, &user_proc](){
        while(user_proc.is_alive()){
            while(chunk2->read_tail != chunk2->write_tail){
                // There are packets needing to be routed
                packet read_packet = chunk2->payloads[chunk2->read_tail & 63];
                chunk2->read_tail.fetch_add(1);

                // Simulate latency here
                std::thread([read_packet, &user_proc](){
                    U64 latency = (mag(user_proc.get_position_as_vector(read_packet.target_user) - user_proc.get_sat_position_as_vector(read_packet.target_sat)) / SOL) * 1000.0f;

                    if(latency == 0) latency = 1;

                    std::this_thread::sleep_for(std::chrono::milliseconds(latency));

                    out("[User Worker] [Incoming message for: [", read_packet.target_user, "] from: [", read_packet.source_user, "]]: ", std::string(read_packet.msg));
                }).detach();
            }

            std::this_thread::yield();
        }
    }).detach();

    out("[User Worker] Ready.");

    cli_prompt_hook = [&user_proc]() {
        int acting_user = user_proc.get_acting_user();
        if (acting_user == -1) {
            return std::string(">> ");
        }
        return "[" + std::to_string(acting_user) + "] >> ";
    };

    std::string opt;

    while(std::cin >> opt) {
        if(opt == "exit") break;
        if(opt == "ssh"){
            int32_t user;
            std::cin >> user;

            user_proc.ssh(user);
        } else if(opt == "get_sat"){
            // Return the SSH'd user's optimal satellite
            U16 user_index;

            if(user_proc.get_acting_user() == -1){
                // We'll want to calculate a basic optimal satellite given the user as input
                std::cin >> user_index;
            } else{
                user_index = user_proc.get_acting_user();
            }

            int32_t optimal_sat = user_proc.get_optimal_sat(user_index); // -1 for no satellites, otherwise contains the satellite's ID

            if(optimal_sat == -1){
                out("[User Worker] User [", user_index, "]'s has no viable satellite coverage.");
            } else{
                out("[User Worker] User [", user_index, "]'s optimal satellite is satellite [", optimal_sat, "].");
            }
        } else if(opt == "tell"){
            int32_t acting_user = user_proc.get_acting_user();
            if(acting_user == -1){
                out("[User Worker] Please SSH into a user (using SSH <user_id>) before using tell.");
                std::getline(std::cin, opt);
                continue;
            }

            U16 target_user;
            std::string msg;

            std::cin >> target_user;
            std::getline(std::cin, msg);

            U16 next_sat = user_proc.get_optimal_sat(acting_user);

            packet to_push;
            to_push.source_user = acting_user;
            to_push.target_user = target_user;
            to_push.target_sat = user_proc.get_optimal_sat(target_user);
            to_push.next_sat = next_sat;

            std::memcpy(to_push.msg, msg.c_str()+1, msg.length());

            U64 latency = (mag(user_proc.get_sat_position_as_vector(next_sat) - user_proc.get_position_as_vector(acting_user)) / SOL) * 1000.0f;
            if(latency == 0) latency = 1;

            std::thread([latency, chunk1, &to_push](){
                std::this_thread::sleep_for(std::chrono::milliseconds(latency)); // Simulate latency of packet getting up to the satellite

                size_t slot = chunk1->write_tail.fetch_add(1);

                chunk1->packets[slot & 63] = std::move(to_push);
            }).detach();
        }
    }

    return 0;
}