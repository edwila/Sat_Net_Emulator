#pragma once

// Constants used by the program
#include <cstdint> // uint8_t, uint16_t, uint32_t, uint64_t
#include <immintrin.h> // AVX2 register functionality
#include <array> // std::array<>()
#include <thread> // std::thread::Hardware_Concurrency
#include <atomic> // std::atomic
#include <sys/mman.h> // mmap
#include <sys/stat.h>
#include <fcntl.h> // flags
#include <unistd.h> // modes

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
    Vector3 operator-(const Vector3& other){
        return {X-other.X, Y-other.Y, Z-other.Z};
    }
};

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

struct response_struct {
    U16 satellite_id;
    
    float X, Y, Z;
}; // 16 bytes (2 + 4 + 4 + 4 = 2 + 12 = 14 + 2[PADDING] = 16)

// Only station or satellite need to know how this struct is structured
#if defined(SAT_ACCESS) || defined(STATION_ACCESS)
struct satellite_station_container {
    std::array<U16, 64> requests;
    std::array<response_struct, 64> responses;

    alignas(64) std::atomic<U32> req_sat_tail;
    alignas(64) std::atomic<U32> req_station_tail;
    alignas(64) std::atomic<U32> res_station_tail;
    alignas(64) std::atomic<U32> res_sat_tail;

    bool initialized = false;
};
#endif