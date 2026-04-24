#include "../../Dependencies/consts.hpp"
#include <chrono>
#include <vector>
#include <random>
#include <iostream>

using ms = std::chrono::milliseconds;

class User_Processor {
    private:
        std::chrono::steady_clock::time_point start; // Start timestamp
        Users container;
        std::vector<std::thread> threads;
        float time_scalar = 1.0f; // How much faster time moves (sim time)
        bool active;
    public:
        User_Processor(); // Initializes start to be std::chrono::steady_clock::now()
        ~User_Processor();

        ms get_elapsed_time(); // Return the elapsed time since 

        std::tuple<float, float, float> get_position(size_t idx);

        // Populate user container
        void populate(U16 amount);
};