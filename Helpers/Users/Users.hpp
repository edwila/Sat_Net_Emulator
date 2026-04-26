#ifndef USER_ACCESS
#define USER_ACCESS
#endif

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
        Satellites* sat_container;
        std::vector<std::thread> threads;
        float time_scalar = 1.0f; // How much faster time moves (sim time)
        bool active;
        int32_t acting_user; // The user we want to act as. -1 for none.
    public:
        User_Processor(Satellites* ctr); // Initializes start to be std::chrono::steady_clock::now()
        ~User_Processor();

        ms get_elapsed_time(); // Return the elapsed time since 

        std::tuple<float, float, float> get_position(size_t idx);

        void ssh(int32_t target_user);

        int32_t get_acting_user() const;

        int32_t get_optimal_sat(U16 user_index) const;

        // Populate user container
        void populate(U16 amount);
};