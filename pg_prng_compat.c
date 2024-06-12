#if PG_VERSION_NUM < 150000

#include <math.h>

#include "postgres.h"

#include "pg_prng_compat.h"

/*
 * We use this generator just to fill the xoroshiro128** state vector
 * from a 64-bit seed.
 */
static uint64
splitmix64(uint64 *state)
{
	/* state update */
	uint64		val = (*state += UINT64CONST(0x9E3779B97f4A7C15));

	/* value extraction */
	val = (val ^ (val >> 30)) * UINT64CONST(0xBF58476D1CE4E5B9);
	val = (val ^ (val >> 27)) * UINT64CONST(0x94D049BB133111EB);

	return val ^ (val >> 31);
}

/*
 * 64-bit rotate left
 */
static inline uint64
rotl(uint64 x, int bits)
{
	return (x << bits) | (x >> (64 - bits));
}

/*
 * The basic xoroshiro128** algorithm.
 * Generates and returns a 64-bit uniformly distributed number,
 * updating the state vector for next time.
 *
 * Note: the state vector must not be all-zeroes, as that is a fixed point.
 */
static uint64
xoroshiro128ss(pg_prng_state *state)
{
	uint64		s0 = state->s0,
				sx = state->s1 ^ s0,
				val = rotl(s0 * 5, 7) * 9;

	/* update state */
	state->s0 = rotl(s0, 24) ^ sx ^ (sx << 16);
	state->s1 = rotl(sx, 37);

	return val;
}

/*
 * Validate a PRNG seed value.
 */
static bool
pg_prng_seed_check(pg_prng_state *state)
{
	/*
	 * If the seeding mechanism chanced to produce all-zeroes, insert
	 * something nonzero.  Anything would do; use Knuth's LCG parameters.
	 */
	if (unlikely(state->s0 == 0 && state->s1 == 0))
	{
		state->s0 = UINT64CONST(0x5851F42D4C957F2D);
		state->s1 = UINT64CONST(0x14057B7EF767814F);
	}

	/* As a convenience for the pg_prng_strong_seed macro, return true */
	return true;
}

/*
 * Initialize the PRNG state from a 64-bit integer,
 * taking care that we don't produce all-zeroes.
 */
void
pg_prng_seed(pg_prng_state *state, uint64 seed)
{
	state->s0 = splitmix64(&seed);
	state->s1 = splitmix64(&seed);
	/* Let's just make sure we didn't get all-zeroes */
	(void) pg_prng_seed_check(state);
}

/*
 * Select a random uint64 uniformly from the range [0, PG_UINT64_MAX].
 */
uint64
pg_prng_uint64(pg_prng_state *state)
{
	return xoroshiro128ss(state);
}

/*
 * Select a random uint32 uniformly from the range [0, PG_UINT32_MAX].
 */
uint32
pg_prng_uint32(pg_prng_state *state)
{
	/*
	 * Although xoroshiro128** is not known to have any weaknesses in
	 * randomness of low-order bits, we prefer to use the upper bits of its
	 * result here and below.
	 */
	uint64		v = xoroshiro128ss(state);

	return (uint32) (v >> 32);
}

/*
 * Select a random double uniformly from the range [0.0, 1.0).
 *
 * Note: if you want a result in the range (0.0, 1.0], the standard way
 * to get that is "1.0 - pg_prng_double(state)".
 */
double
pg_prng_double(pg_prng_state *state)
{
	uint64		v = xoroshiro128ss(state);

	/*
	 * As above, assume there's 52 mantissa bits in a double.  This result
	 * could round to 1.0 if double's precision is less than that; but we
	 * assume IEEE float arithmetic elsewhere in Postgres, so this seems OK.
	 */
	return ldexp((double) (v >> (64 - 52)), -52);
}
#endif
