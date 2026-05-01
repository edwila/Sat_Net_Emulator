#include "optimal.h"

__attribute__((target("avx2,fma")))
std::vector<U16> solve_helper(const Backend::Vector3& u_pos, const float R_Squared, const float dot_p1, const U16 num_sats, const Coordinates<MAX_SATELLITES>& positions){
    std::vector<U16> sat_ids;
    sat_ids.reserve(num_sats);

    __m256 upos_x = _mm256_set1_ps(u_pos.X); // Smeared u_pos_x
    __m256 upos_y = _mm256_set1_ps(u_pos.Y); // Smeared u_pos_y
    __m256 upos_z = _mm256_set1_ps(u_pos.Z); // Smeared u_pos_z
    __m256 r_sq = _mm256_set1_ps(R_Squared); // Smeared radius squared
    
    __m256 dot_p1_vec = _mm256_set1_ps(dot_p1);
    int k = 0;
    for(; k < num_sats-7; k += 8){ // We can load 8 satellites at a time into our 3 AVX registers
        __m256 sat_x = _mm256_loadu_ps(&positions.X[k]);
        __m256 sat_y = _mm256_loadu_ps(&positions.Y[k]);
        __m256 sat_z = _mm256_loadu_ps(&positions.Z[k]);
        
        // Now our satellite AVX registers are loaded (sats 0-7 for each chunk of 8)
        __m256 prods = _mm256_mul_ps(upos_x, sat_x);
        prods = _mm256_fmadd_ps(upos_y, sat_y, prods);
        prods = _mm256_fmadd_ps(upos_z, sat_z, prods);

        // Prods now contains the result of the 8 dot products, so let's check them

        __m256 mask = _mm256_cmp_ps(prods, r_sq, _CMP_GE_OQ);

        // Let's calculate the beam and load it into the AVX register. beam = *(sats+k) - u_pos
        __m256 beam_x = _mm256_sub_ps(sat_x, upos_x);
        __m256 beam_y = _mm256_sub_ps(sat_y, upos_y);
        __m256 beam_z = _mm256_sub_ps(sat_z, upos_z);

        // We want dot_prod(u_pos, beam), which is _mm256_mul_ps(upos_x, beam_x)
        prods = _mm256_mul_ps(upos_x, beam_x);
        prods = _mm256_fmadd_ps(upos_y, beam_y, prods);
        prods = _mm256_fmadd_ps(upos_z, beam_z, prods);

        __m256 beam_sq = _mm256_mul_ps(beam_x, beam_x);
        beam_sq = _mm256_fmadd_ps(beam_y, beam_y, beam_sq);
        beam_sq = _mm256_fmadd_ps(beam_z, beam_z, beam_sq);

        // Validate the prods as long as it's greater than 0 (so we don't lose our sign)

        mask = _mm256_and_ps(mask, _mm256_cmp_ps(prods, _mm256_setzero_ps(), _CMP_GT_OQ));

        // We wanna calculate prods*prods and compare it to dot_p1_vec _mm256_mul with mag_sq of beam

        prods = _mm256_mul_ps(prods, prods);

        mask = _mm256_and_ps(mask, _mm256_cmp_ps(prods, _mm256_mul_ps(dot_p1_vec, beam_sq), _CMP_GE_OQ));

        uint8_t casted_mask = _mm256_movemask_ps(mask);

        while(casted_mask != 0){
            // Now do the same vectorized dot product calculation but for the angle
            
            int index = __builtin_ctz(casted_mask); // This is the satellite that's available to us

            // sats+k+index is our valid satellite!

            sat_ids.emplace_back(k+index);

            casted_mask &= (casted_mask - 1);
        }
    }

    // Catch-up check : what happens if we have a number of satellites that's not divisible by 8? This loop will take care of them
    // We can't really include them into the SIMD logic because then we are basically reintroducing unaligned access
    // + gonna be shifting to chunk the satellites
    for(; k < num_sats; k++){
        Backend::Vector3 s_pos = {positions.X[k], positions.Y[k], positions.Z[k]};
        if(dot_func(u_pos, s_pos) < R_Squared) continue;
        Backend::Vector3 beam = s_pos - u_pos;
        float dot_val = dot_func(u_pos, beam);
        
        if(dot_val > 0.0f && (dot_val * dot_val) >= (dot_p1 * mag_sq(beam))) {
            sat_ids.emplace_back(k);
        }
    }

    return sat_ids;
}

std::vector<U16> optimal_sats(const Backend::Vector3& user, const Satellites* sats)
{
    static const float R_Squared = mag_sq(user); // Radius of earth squared. Used to cull users past the horizon

    U16 num_sats = sats->initialized_satellites;

    Coordinates<MAX_SATELLITES> positions = sats->positions;

    std::vector<U16> to_ret = solve_helper(user, R_Squared, (COS_MAX_A_SQUARED * mag_sq(user)), num_sats, positions);

    std::sort(to_ret.begin(), to_ret.end(), [&](U16 a, U16 b){
        return mag_sq(Backend::Vector3{positions.X[a], positions.Y[a], positions.Z[a]}-user) < mag_sq(Backend::Vector3{positions.X[b], positions.Y[b], positions.Z[b]}-user);
    });

    return to_ret;
}