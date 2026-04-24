// Constants used by the program
#include <cstdint> // uint8_t, uint16_t, uint32_t, uint64_t
#include <immintrin.h> // AVX2 register functionality
#include <array> // std::array<>()
#include <thread> // std::thread::Hardware_Concurrency

// Types
using U8 = uint8_t;
using U16 = uint16_t;
using U32 = uint32_t;
using U64 = uint64_t;

static unsigned int MAX_THREADS = std::thread::hardware_concurrency() == 0 ? 16 : std::thread::hardware_concurrency();

// Constants
static constexpr U32 SOL = 299792458; // Speed of light (meters/s)
static constexpr U16 MAX_USERS = 0xFFFF; // Max user connectivity (65536 * 32[size of float] = 2097152 bits, or 262144 bytes, enough to fit in an entire L2 cache)
static constexpr U16 MAX_SATELLITES = 0xFFFF; // Maximum number of satellites allowed
static constexpr U32 LEO_ALTITUDE = 6921000; // Altitude of LEO satellites (used in random population) in meters
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