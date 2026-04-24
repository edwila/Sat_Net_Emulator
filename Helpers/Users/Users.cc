#include "Users.hpp"

User_Processor::User_Processor(){
    start = std::chrono::steady_clock::now();
    threads.reserve(MAX_THREADS);
};

User_Processor::~User_Processor(){
    active = false;
    for(std::thread& t : threads){
        if(t.joinable()) t.join();
    }
}

ms User_Processor::get_elapsed_time(){
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start);
}

std::tuple<float, float, float> User_Processor::get_position(size_t idx){
    return {
        container.positions.X[idx],
        container.positions.Y[idx],
        container.positions.Z[idx]
    };
}

// Populate user container
void User_Processor::populate(U16 amount = 0xFFFF) {
    active = true;
    container.initialized_users = amount;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> theta_dist(0.0f, 2.0f * PI);
    std::uniform_real_distribution<float> phi_dist(0.0f, PI); // Only need 0-pi because it would just wrap around the poles
    std::uniform_real_distribution<float> t_dist(-1.0f, 1.0f);


    U16 batch_size = amount/MAX_THREADS;

    for(U16 i = 0; i < amount; i++){
        // Populate the users container
        // Polar coordinate:
        /*
        x: R_EARTH * cos(random_theta) * sin(random_phi)
        y: R_EARTH * sin(random_theta) * sin(random_phi)
        z: R_EARTH * cos(random_phi)
        */

        // TODO: SIMD HERE TOO

        float random_theta = theta_dist(gen);
        float random_phi = phi_dist(gen);

        float sin_phi = std::sin(random_phi), cos_phi = std::cos(random_phi);

        float x = R_EARTH * std::cos(random_theta) * sin_phi, 
        y = R_EARTH * std::sin(random_theta) * sin_phi, 
        z = R_EARTH * cos_phi;

        float T_x = t_dist(gen);
        float T_y = t_dist(gen);
        float T_z = t_dist(gen);

        container.positions.X[i] = x;
        container.positions.Y[i] = y;
        container.positions.Z[i] = z;

        std::cout << "[User " << i << "]: <" << x << ", " << y << ", " << z << "> [" << std::sqrt(x*x + y*y + z*z) << "]\n";
    }
    
    std::cout << ">> ";
};