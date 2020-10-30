# random generator extension

This PostgreSQL extension implements a couple of functions to generate
random values for various data types, which may be useful for testing
purposes etc.


## Functions

### `random_string(length)`

Generates a string with the specified length, using ASCII characters,
symbols and control characters (newline, carriage return, tabulator).


### `random_bytea(length)`

Generates a bytea with the specified length.


### `random_int(min_value, max_value)`

Generates a random 32-bit integer in the specified interval.


### `random_bigint(min_value, max_value)`

Generates a random 64-bit integer in the specified interval.


### `random_real(min_value, max_value)`

Generates a random 32-bit floating point value in the specified
interval.


### `random_double_recision(min_value, max_value)`

Generates a random 64-bit floating point value in the specified
interval.


License
-------
This software is distributed under the terms of PostgreSQL license.
See LICENSE or http://www.opensource.org/licenses/bsd-license.php for
more details.
