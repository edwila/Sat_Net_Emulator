#include "Users.hpp"

User_Processor::User_Processor(Satellites* ctr){
    sat_container = ctr;

    acting_user = -1;

    start = std::chrono::steady_clock::now();
    threads.reserve(MAX_THREADS);
};

User_Processor::~User_Processor(){
    active = false;
    for(std::thread& t : threads){
        if(t.joinable()) t.join();
    }
}

int32_t User_Processor::get_acting_user() const {
    return acting_user;
}

int32_t User_Processor::get_optimal_sat(U16 user_index, bool ensure_connection) const {
    // If ensure_connection is true, this call will yield if there are no optimal satellites, repeatedly checking until there is a valid sat
    // We want to return the optimal satellite ID for U16
    // Recall that all satellites are stored at sat_container

    std::vector<U16> optimals = optimal_sats(Vector3{
        container.positions.X[user_index],
        container.positions.Y[user_index],
        container.positions.Z[user_index]
    }, sat_container);

    if(ensure_connection && optimals.empty()){
        while(optimals.empty()){
            optimals = optimal_sats(Vector3{
                container.positions.X[user_index],
                container.positions.Y[user_index],
                container.positions.Z[user_index]
            }, sat_container);
        }
    }

    return optimals.empty() ? -1 : optimals.front();
}

U32 User_Processor::get_elapsed_time(){
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
}

std::tuple<float, float, float> User_Processor::get_position(size_t idx){
    return {
        container.positions.X[idx],
        container.positions.Y[idx],
        container.positions.Z[idx]
    };
}

Vector3 User_Processor::get_position_as_vector(size_t idx){
    return Vector3{
        container.positions.X[idx],
        container.positions.Y[idx],
        container.positions.Z[idx]
    };
}

void User_Processor::ssh(int32_t target_user){
    acting_user = target_user;

    if(acting_user == -1){
        std::cout << "[User Worker] Disconnected from user SSH.\n";
    } else{
        std::cout << "[User Worker] SSH'd into user " << acting_user << "!\n";
    }
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
};