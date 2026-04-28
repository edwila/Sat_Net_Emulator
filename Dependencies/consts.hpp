#pragma once

// Constants used by the program
#include <cstdint> // uint8_t, uint16_t, uint32_t, uint64_t
#include <immintrin.h> // AVX2 register functionality
#include <array> // std::array<>()
#include <thread> // std::thread::Hardware_Concurrency
#include <atomic> // std::atomic
#include <sys/mman.h> // mmap
#include <sys/stat.h>
#include <cstring> // std::memcpy
#include <fcntl.h> // flags
#include <unistd.h> // modes
#include <cmath> // std::cos
#include <queue> // std::priority_queue
#include <utility> // std::pair
#include <queue> // std::queue
#include <iostream> // std::cout, std::cin
#include <functional> // std::function

// Types
using U8 = uint8_t;
using U16 = uint16_t;
using U32 = uint32_t;
using U64 = uint64_t;

inline unsigned int MAX_THREADS = std::thread::hardware_concurrency() == 0 ? 16 : std::thread::hardware_concurrency();

// Constants
static constexpr U32 SOL = 299792458; // Speed of light (meters/s)
static constexpr U16 MAX_USERS = 65528; // Max user connectivity (65536 * 32[size of float] = 2097152 bits, or 262144 bytes, enough to fit in an entire L2 cache)
static constexpr U16 MAX_SATELLITES = 65528; // Maximum number of satellites allowed
static constexpr U32 R_EARTH = 6378100; // Radius of the Earth
static constexpr U32 LEO_ALTITUDE = 542900; // Low Earth Orbit altitude (R_EARTH + LEO_ALTITUDE)
static constexpr float PI = 3.1415926;
static constexpr float mu = 3.986004418e14;

// Usables
struct Vector3 {
    float X, Y, Z;
    Vector3 operator-(const Vector3& other) const {
        return {X-other.X, Y-other.Y, Z-other.Z};
    };
    Vector3 operator+(const Vector3& other) const {
        return {X+other.X, Y+other.Y, Z+other.Z};
    };
    Vector3 operator*(float other) const {
        return {X*other, Y*other, Z*other};
    };
    Vector3 operator/(float other) const {
        return {X/other, Y/other, Z/other};
    };
};

inline std::function<std::string()> cli_prompt_hook = []() { return ">> "; };

template<typename... Args>
inline void out(Args&&... args){
    std::cout << "\r\033[2K";
    ((std::cout << std::forward<Args>(args)), ...);
    std::cout << "\n" << cli_prompt_hook() << std::flush;
}

template <U16 max_size>
struct Coordinates {
    std::array<float, max_size> X;
    std::array<float, max_size> Y;
    std::array<float, max_size> Z;
};

struct Users {
    // Struct to hold user information
    // SoA structure using MAX_USERS for optimal spatial cache locality
    Coordinates<MAX_USERS> positions;

    U16 initialized_users = 0;
};

struct Satellites {
    // Similar to users struct, this struct holds (and maintains) satellite information
    Coordinates<MAX_SATELLITES> positions;
    Coordinates<MAX_SATELLITES> velocities; // Satellite velocities (assuming 0 acceleration besides gravity)

    U16 initialized_satellites = 0;
};

using r_t = std::array<std::array<U16, MAX_SATELLITES>, MAX_SATELLITES>;

struct routing_table {
    alignas(64) std::atomic<U8> routing_table_key{0}; // 0 - Table A is active, 1 - Table B is active

    r_t table_A;
    r_t table_B;
};

struct packet {
    // Network packet. Used for users communicating with other users
    char msg[64];

    U16 source_user, target_user;
    
    U16 next_sat; // We need to store the next hop because we can't just place this packet into the satellite's buffer

    // The satellite we want to target
    U16 target_sat;
    bool completed = false; // True if it arrives at its destination satellites and needs to be sent to its user now

    bool operator<(const packet& other) const {
        return target_sat < other.target_sat; 
    };
    
    bool operator>(const packet& other) const {
        return target_sat > other.target_sat;
    };
};

// Only station or satellite need to know how this struct is structured
struct shared_mem_container {
    Satellites container;

    std::array<packet, 64> packets; // Array of packets. Simulates the packet sent from a user to a satellite

    alignas(64) std::atomic<U32> read_tail{0};
    alignas(64) std::atomic<U32> write_tail{0};

    routing_table table;

    bool initialized = false;
};

struct user_sat_mem {
    std::array<packet, 64> payloads;

    alignas(64) std::atomic<U32> read_tail{0};
    alignas(64) std::atomic<U32> write_tail{0};
};

inline float mag_sq(const Vector3& x){
    return (x.X*x.X + x.Y*x.Y + x.Z*x.Z);
};

inline float mag(const Vector3& x){
    return std::sqrt(mag_sq(x));
};

inline float dot_func(const Vector3& a, const Vector3& b){
    return (a.X * b.X + a.Y * b.Y + a.Z * b.Z);
};

using time_pq = std::priority_queue<std::pair<U64, packet>, std::vector<std::pair<U64, packet>>, std::greater<std::pair<U64, packet>>>;