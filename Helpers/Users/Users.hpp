#ifndef USER_ACCESS
#define USER_ACCESS
#endif

#include "../../Dependencies/consts.hpp"
#include "../../Dependencies/optimal.h"
#include <chrono>
#include <vector>
#include <random>
#include <iostream>

static unsigned int MAX_THREADS = std::thread::hardware_concurrency() == 0 ? 16 : std::thread::hardware_concurrency();

class User_Processor {
    private:
        std::chrono::steady_clock::time_point start; // Start timestamp
        Users* container;
        Satellites* sat_container;
        std::vector<std::thread> threads;
        float time_scalar = 1.0f; // How much faster time moves (sim time)
        bool active;
        int32_t acting_user; // The user we want to act as. -1 for none.
        time_pq latency_emulator;
        std::queue<packet> waiting_to_send;
    public:
        User_Processor(Satellites* ctr, Users* users); // Initializes start to be std::chrono::steady_clock::now()
        ~User_Processor();

        U32 get_elapsed_time(); // Return the elapsed time since 

        std::tuple<float, float, float> get_position(size_t idx);

        Vector3 get_position_as_vector(size_t idx);

        Vector3 get_sat_position_as_vector(size_t idx);

        void ssh(int32_t target_user);

        int32_t get_acting_user() const;

        bool is_alive() const;

        time_pq* get_latency();

        int32_t get_optimal_sat(U16 user_index, bool ensure_connection = false) const;

        // Populate user container
        void populate(U16 amount);
};