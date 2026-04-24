#include "../Helpers/Satellites.hpp"
#include <iostream>
#include <string>

int main(){
    Satellite_Processor proc;

    U16 num_sats;

    std::string opt;

    

    std::cout << "Enter number of satellites: ";
    std::cin >> num_sats;

    std::cout << ">> ";

    proc.populate(num_sats);

    std::cout << "Populated!\n>> ";

    while(std::cin >> opt){
        if(opt == "exit") break;
        if(opt == "print"){
            int idx;
            std::cin >> idx;

            auto [x, y, z] = proc.get_position(idx);

            std::cout << "Satellite [" << idx << "] : <" << x << ", " << y << ", " << z << "> [" << std::sqrt(x*x + y*y + z*z) << "]\n>> ";
        }
    }
    
    return 0;
};