#include "Users.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    U16 num_users = 0xFFFF; 
    if (argc > 1) {
        num_users = std::stoi(argv[1]);
    }

    std::cout << "[User Worker] Booting up with " << num_users << " users...\n";

    User_Processor user_proc;
    user_proc.populate(num_users);

    while(true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
    }

    return 0;
}