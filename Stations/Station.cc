#include "../Dependencies/consts.hpp"
#include <iostream>
#include <string>

int main(){
    U16 num_sats, num_users;    

    std::cout << "Enter number of satellites: ";
    std::cin >> num_sats;

    std::cout << "Enter number of users: ";
    std::cin >> num_users;

    std::cout << "Launching workers...\n";

    // Reserve shared memory here
    system(("cmd.exe /c start wsl ./satellite_worker " + std::to_string(num_sats)).c_str());
    system(("cmd.exe /c start wsl ./user_worker " + std::to_string(num_users)).c_str());

    std::cout << "Workers launched!\n";
    
    return 0;
};