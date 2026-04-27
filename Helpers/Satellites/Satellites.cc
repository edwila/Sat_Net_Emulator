#include "Satellites.hpp"

Satellite_Processor::Satellite_Processor(Satellites* ctr){
    container = ctr;
    active = true;
    start = std::chrono::steady_clock::now();
    threads.reserve(MAX_THREADS);
};

Satellite_Processor::~Satellite_Processor(){
    kill();
};

void Satellite_Processor::kill(){
    active = false;

    for(std::thread& t : threads){
        if(t.joinable()) t.join();
    }
};

bool Satellite_Processor::is_alive() const {
    return active;
};

U32 Satellite_Processor::get_elapsed_time(){
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
};

std::tuple<float, float, float> Satellite_Processor::get_position(size_t idx){
    return {
        container->positions.X[idx],
        container->positions.Y[idx],
        container->positions.Z[idx]
    };;
};

__attribute__((target("avx2,fma")))
void Satellite_Processor::sat_helper(float dt, U16 batch_size, unsigned int thread_index, U16 end_idx_for_thread){
    U16 sat_idx = batch_size*thread_index;

    for(; sat_idx < end_idx_for_thread-7; sat_idx += 8){
        // Calculate position of satellite at index sat_idx (use SIMD)
        // a = -mu/(|p|^3) * p : gravitational force
        // steps:
        // calculate a
        // v = v_cur + a*dt
        // p = p_cur + v*dt
        // use SIMD to calculate 8 x's, 8 y's, and 8 z's

        __m256 p_x = _mm256_loadu_ps(&container->positions.X[sat_idx]);
        __m256 p_y = _mm256_loadu_ps(&container->positions.Y[sat_idx]);
        __m256 p_z = _mm256_loadu_ps(&container->positions.Z[sat_idx]);
        // Now we have 8 initial positions loaded into 3 AVX registers

        __m256 v_x = _mm256_loadu_ps(&container->velocities.X[sat_idx]);
        __m256 v_y = _mm256_loadu_ps(&container->velocities.Y[sat_idx]);
        __m256 v_z = _mm256_loadu_ps(&container->velocities.Z[sat_idx]);
        // And 8 velocities

        __m256 dt_avx = _mm256_set1_ps(dt);

        // Now we need r^2
        __m256 r2 = _mm256_fmadd_ps(p_x, p_x, _mm256_fmadd_ps(p_y, p_y, _mm256_mul_ps(p_z, p_z)));

        // 1/sqrt(r) => hardware instruction => FAST!
        __m256 inv_r = _mm256_rsqrt_ps(r2);
        __m256 inv_r3 = _mm256_mul_ps(inv_r, _mm256_mul_ps(inv_r, inv_r));

        __m256 g = _mm256_mul_ps(_mm256_mul_ps(_mm256_set1_ps(-mu), dt_avx), inv_r3);

        v_x = _mm256_fmadd_ps(p_x, g, v_x);
        v_y = _mm256_fmadd_ps(p_y, g, v_y);
        v_z = _mm256_fmadd_ps(p_z, g, v_z);

        p_x = _mm256_fmadd_ps(v_x, dt_avx, p_x);
        p_y = _mm256_fmadd_ps(v_y, dt_avx, p_y);
        p_z = _mm256_fmadd_ps(v_z, dt_avx, p_z);

        // storeu velocities B)
        _mm256_storeu_ps(&container->velocities.X[sat_idx], v_x);
        _mm256_storeu_ps(&container->velocities.Y[sat_idx], v_y);
        _mm256_storeu_ps(&container->velocities.Z[sat_idx], v_z);

        // storeu positions B)
        _mm256_storeu_ps(&container->positions.X[sat_idx], p_x);
        _mm256_storeu_ps(&container->positions.Y[sat_idx], p_y);
        _mm256_storeu_ps(&container->positions.Z[sat_idx], p_z);
    }

    for(; sat_idx < end_idx_for_thread; sat_idx++) {
        float px = container->positions.X[sat_idx];
        float py = container->positions.Y[sat_idx];
        float pz = container->positions.Z[sat_idx];

        float r2 = px*px + py*py + pz*pz;
        float inv_r3 = 1.0 / (std::sqrt(r2) * r2);
        float g_dt = -mu * dt * inv_r3;

        container->velocities.X[sat_idx] += px * g_dt;
        container->velocities.Y[sat_idx] += py * g_dt;
        container->velocities.Z[sat_idx] += pz * g_dt;

        container->positions.X[sat_idx] += container->velocities.X[sat_idx] * dt;
        container->positions.Y[sat_idx] += container->velocities.Y[sat_idx] * dt;
        container->positions.Z[sat_idx] += container->velocities.Z[sat_idx] * dt;
    }
}

// Populate satellite container
void Satellite_Processor::populate(U16 amount = 0xFFFF) {
    active = true;
    container->initialized_satellites = amount;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> theta_dist(0.0f, 2.0f * PI);
    std::uniform_real_distribution<float> phi_dist(0.0f, PI); // Only need 0-pi because it would just wrap around the poles
    std::uniform_real_distribution<float> t_dist(-1.0f, 1.0f);


    U16 batch_size = amount/MAX_THREADS;

    U32 LEO_RAD = R_EARTH + LEO_ALTITUDE;

    for(U16 i = 0; i < amount; i++){
        // Populate the satellites container
        // Polar coordinate:
        /*
        x: LEO_RAD * cos(random_theta) * sin(random_phi)
        y: LEO_RAD * sin(random_theta) * sin(random_phi)
        z: LEO_RAD * cos(random_phi)
        */

        float random_theta = theta_dist(gen);
        float random_phi = phi_dist(gen);

        float sin_phi = std::sin(random_phi), cos_phi = std::cos(random_phi);

        float x = LEO_RAD * std::cos(random_theta) * sin_phi, 
        y = LEO_RAD * std::sin(random_theta) * sin_phi, 
        z = LEO_RAD * cos_phi;

        float T_x = t_dist(gen);
        float T_y = t_dist(gen);
        float T_z = t_dist(gen);

        container->positions.X[i] = x;
        container->positions.Y[i] = y;
        container->positions.Z[i] = z;

        // Initial velocity = <0, 0, 0> => Falling towards the center of the Earth
        // TODO: Adjust to be orthogonal to satellite's position vector
        container->velocities.X[i] = container->velocities.Y[i] = container->velocities.Z[i] = 0;

        std::cout << "[Satellite " << i << "]: <" << x << ", " << y << ", " << z << "> [" << std::sqrt(x*x + y*y + z*z) << "]\n";
    }

    // Batch satellites into threads here, and compute their orbital math using SIMD
    // This will allow each thread to process the orbital mechanics of 8 satellites simultaneously
    // considering each satellite's components are floats (32 bits) and our AVX registers are 256 bits wide
    
    for(unsigned int thread_index = 0; thread_index < MAX_THREADS; thread_index++){
        threads.emplace_back([this, thread_index, amount, batch_size](){
            U16 end_idx_for_thread = thread_index == MAX_THREADS-1 ? amount : batch_size*(thread_index+1);
            std::chrono::steady_clock::time_point last = std::chrono::steady_clock::now();
            while(active){
                std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
                float dt = std::chrono::duration<float>(now-last).count() * time_scalar;
                
                sat_helper(dt, batch_size, thread_index, end_idx_for_thread);

                last = now;

                std::this_thread::yield();
            }
        });
    }
};