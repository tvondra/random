# random generator extension

[![make installcheck](https://github.com/tvondra/random/actions/workflows/ci.yml/badge.svg)](https://github.com/tvondra/random/actions/workflows/ci.yml)

This PostgreSQL extension implements a couple of functions to generate
random values for various data types, which may be useful for testing
purposes etc.

## Reproducible output

To allow generating repeatable data sets, all functions take a `seed` and
`nvalues` parameters. The `seed` determines a subset of possible values to
generate, while `nvalues` determines the number of distinct values.

You can imagine this as as a random number generator initialized with

```
(seed, k)
```

where `k` is one of the `nvalues` values, picked randomly. The generator
is reseeded after every single value. Alternatively, this can be seen as
a pair of random number generators `PRNG1` and `PRNG2`, and each function
call does this (in pseudocode):

```
/* get K from global PRNG */
K = generate_value(PRNG1) % nvalues;

/* seed the "local" PRNG with a combination of seed and K */
set_seed(PRNG2, seed + K);

/* generate a value from the "local" PRNG (can be more complex) */
return generate_value(PRNG1);
```

Note: This does not necessarily mean data sets generated using the same
`seed` will be exactly same. It will contain the same sets of values, but
the order of values may be different, the combinations of values inserted
into different columns of a table may be different, etc.

Note: The actual number of distinct values generated may be lower than
`nvalues`, due to collisions of the PRNG. The code does not attempt for
correct for this.

## Functions

### `random_string(seed, nvalues, min_length, max_length)`

Generates a string with the specified length, using ASCII characters,
symbols and control characters (newline, carriage return, tabulator).


### `random_bytea(seed, nvalues, min_length, max_length)`

Generates a bytea with the specified length.


### `random_int(seed, nvalues, min_value, max_value)`

Generates a random 32-bit integer in the specified interval.


### `random_bigint(seed, nvalues, min_value, max_value)`

Generates a random 64-bit integer in the specified interval.


### `random_real(seed, nvalues, min_value, max_value)`

Generates a random 32-bit floating point value in the specified
interval.


### `random_double_recision(seed, nvalues, min_value, max_value)`

Generates a random 64-bit floating point value in the specified
interval.


### `random_inet(seed, nvalues)`

Generates a random `INET` address. The generated addresses have
`/32` mask.


### `random_cnet(seed, nvalues)`

Generates a random `CNET` address. The generated addresses have
masks set to `8`, `16`, `24` or `32`, and the addresses are picked
with the same probability. This means that addresses with short
masks are much less common in the output - for every `/8` address
we expect 16M `/32` addresses.


### `random_cnet2(seed, nvalues)`

Generates a random `CNET` address. The generated addresses have
masks set to `8`, `16`, `24` or `32`, but the addresses don't have
the same probability. Instead, data is generates so that each mask
length represents the same fraction of the produced values (1/4 for
each mask length).

This however means that longer masks will have namy more distinct
values, to compensate for this skew (the shorter the mask, the
fewer distinct values it can accomodate).


### `random_macaddr(seed, nvalues)`

Generates a random MACADDR value (6B variant).


### `random_macaddr8(seed, nvalues)`

Generates a random MACADDR8 value (8B variant).


License
-------
This software is distributed under the terms of PostgreSQL license.
See LICENSE or http://www.opensource.org/licenses/bsd-license.php for
more details.
