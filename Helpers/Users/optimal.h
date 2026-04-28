#include "../../Dependencies/consts.hpp"

#include <vector> // std::vector
#include <fstream> // ofstream
#include <array> // std::array<>
#include <algorithm> // std::sort

static constexpr U8 MAX_BEAMS = 32;
static constexpr U8 MAX_VISIBLE_SATS = 128;
static constexpr float MAX_ANGLE = 45.0f * (PI/180);

// Calculating acos() for each angle just so it's readable for us (humans) is HIGHLY inefficient for the computer
// so let's just pre-compute the cos angle ahead of time and compare it with the dot product
// (given the definition of the dot product of A•B = |A||B|cos(theta))
// (and since we know that cos() decreases monotonically from 0-180 degrees (0-pi))
static const float COS_MAX_A = std::cos(MAX_ANGLE);

static const float COS_MAX_A_SQUARED = COS_MAX_A * COS_MAX_A;
// Logic for above consts:
/*

We know that A•B = |A||B|cos(t), where t = angle between A and B
|A| = sqrt(A.x*A.x + A.y*A.y + A.z*A.z)
|B| = sqrt(B.x*B.x + B.y*B.y + B.z*B.z)

So what if we square both sides to eliminate the sqrt() call?
(A•B)^2 = |A|^2|B|^2cos^2(t)

Now we know that cos(t) decreases as we go from 0 degrees to 180 degrees.
This means that cos(t) > cos(g) being true means that the angle t < g,
where g can be some threshold we set (in this case, 45 degrees and 10 degrees).

with this logic, we only need to check if (A•B)^2 >= (A•B)^2 [WHERE THE ANGLE BETWEEN A AND B IS <= 45 OR <= 10]
If we want to calculate the dot product with our predicate, g, then we simply calculate |A|^2|B|^2cos^2(45[OR 10]).

This completely eliminates the need for acos() and for sqrt(), both of which are expensive calls that require multiple cycles,
and we can still check if the angle is less:

(A•B)^2 >= |A|^2|B|^2cos^2(45) means that the angle between A and B is less than or equal to 45 degrees.

The only issue?
Squaring gets rid of the sign. So the angle between A and B can be 170 degrees, and their dot product squared will be equal to
the dot product squared of the vectors if they were 30 degrees, which would pass. To solve this issue, simply check if the dot product
is greater than or equal to 0 prior to squaring it (negative and zero dot products are >= 90 degrees = invalid). If greater than 0,
square and do the remaining checks.

*/


using user_ctr = std::vector<std::vector<U8>>;

struct user_container {
    // Will contain necessary information about users and the satellites available to each user
    std::vector<U8> avail_sats; // Available satellites for user index i (1D vector, num_users*num_sats size assuming worst case num_sats available for each user (refined via avail_counts vector))
    std::vector<U16> avail_counts; // Count of available satellites for user index i

    std::vector<U8> idxs; // Value at each user indexed at i is their original index
    // Optimal for cache locality since we're sorting the indices rather than the vectors (from avail_sats)

    user_container(const int num_users){
        avail_sats.resize(num_users * MAX_VISIBLE_SATS);
        avail_counts.resize(num_users, 0);

        idxs.resize(num_users);
    };
};

// Generate a set of legal beams to cover as many users as possible.
std::vector<U16> optimal_sats(const Vector3& user, const Satellites* sats);