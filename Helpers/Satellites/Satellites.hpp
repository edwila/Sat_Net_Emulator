#ifndef SAT_ACCESS
#define SAT_ACCESS
#endif

#include "../../Dependencies/consts.hpp"
#include <chrono>
#include <vector>
#include <random>
#include <iostream>

using ms = std::chrono::milliseconds;

class Satellite_Processor {
    private:
        std::chrono::steady_clock::time_point start; // Start timestamp
        Satellites* container;
        std::vector<std::thread> threads;
        float time_scalar = 1.0f; // How much faster time moves (sim time)
        bool active;
        void sat_helper(float dt, U16 batch_size, unsigned int thread_index, U16 end_idx_for_thread);
    public:
        Satellite_Processor(Satellites* ctr); // Initializes start to be std::chrono::steady_clock::now()
        ~Satellite_Processor();

        ms get_elapsed_time(); // Return the elapsed time since firing up the satellite processor

        std::tuple<float, float, float> get_position(size_t idx);

        void kill();

        bool is_alive() const;

        // Populate satellite container
        void populate(U16 amount);
};