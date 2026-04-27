#include "Users.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    U16 num_users = 0xFFFF; 
    if (argc > 1) {
        num_users = std::stoi(argv[1]);
    }

    std::cout << "[User Worker] Booting up with " << num_users << " users...\n";

    int global_rf_space_fd = shm_open("/global_rf_space", O_CREAT | O_RDWR, 0600);
    // This shared memory will be for satellites communicating with the station (satellite requesting routing table, station providing routing table)

    unsigned long long len = sizeof(shared_mem_container);

    int truncate_result = ftruncate(global_rf_space_fd, len);

    if(truncate_result == -1){
        std::cout << "[User Worker] Failed to truncate fd [" << global_rf_space_fd << "]! Err: " << errno << "\n";

        return 1;
    }

    shared_mem_container* chunk1 = (shared_mem_container*)mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED, global_rf_space_fd, 0);

    std::cout << "[User Worker] Shared memory allocated successfully.\n";

    User_Processor user_proc(&chunk1->container);
    user_proc.populate(num_users);

    std::string opt;

    std::cout << ">> ";

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
                std::cout << "[User Worker] User [" << user_index << "]'s has no viable satellite coverage.\n";
            } else{
                std::cout << "[User Worker] User [" << user_index << "]'s optimal satellite is satellite [" << optimal_sat << "]\n";
            }
        } else if(opt == "tell"){
            int32_t acting_user = user_proc.get_acting_user();
            if(acting_user == -1){
                std::cout << "[User Worker] Please SSH into a user (using SSH <user_id>) before using tell.\n>> ";
                std::getline(std::cin, opt);
                continue;
            }

            U16 target_user;
            std::string msg;

            std::cin >> target_user;
            std::getline(std::cin, msg);

            packet to_push;
            to_push.source_user = acting_user;
            to_push.target_user = target_user;
            to_push.target_sat = user_proc.get_optimal_sat(target_user);
            to_push.next_sat = user_proc.get_optimal_sat(acting_user);

            std::memcpy(to_push.msg, msg.c_str()+1, msg.length());

            chunk1->packets[chunk1->write_tail & 63] = std::move(to_push);
            chunk1->write_tail.fetch_add(1);
        }

        std::cout << (user_proc.get_acting_user() == -1 ? "" : ("[" + std::to_string(user_proc.get_acting_user()) + "] ")) << ">> ";
    }

    return 0;
}