# WFLCG

This is a small and simple single-header pseudorandom number generator for C++11 and C99 which
aims to be as fast as possible while still giving decent-quality random numbers. The
main usage scenario is applications, like for example simulations, that require billions
of pseudorandom numbers as efficiently as possible.

`WFLCG` meets the requirements of _UniformRandomBitGenerator_ (and thus can be used anywhere
where a C++ standard library RNG can). It generates 32-bit unsigned integers (and has
versions that return floats and doubles, see below). The period is 2<sup>36</sup>.

A C version of the library is also provided in the `WFLCG_c.h` header file.

While `WFLCG` is LCG-based, and is not cryptographically strong, it is nevertheless of
higher quality than a simple LCG. It passes all Testu01 SmallCrush tests with flying
colors. It's significantly faster than any of the standard C++ random number generators.

## Benchmarks

The benchmark was run by, essentially, using this kind of loop:

```c++
unsigned sum = 0;

for(std::size_t i = 0; i < 1000000000; ++i)
    sum += rng();

// Assign to a volatile to stop the compiler from
// optimizing the whole calculation away:
gValueSink = sum;
```

The benchmark was run on Windows using mingw64 (gcc 7.1.0) with options `-Ofast -march=skylake`
on an i7-9700K, using all the standard library RNGs and `WFLCG`. The measured result is the total
runtime (thus a lower value is better):

| Random number generator | Total time (seconds) |
|:-----------------------:|:--------------------:|
| `std::minstd_rand0` | 3.60 |
| `std::minstd_rand` | 3.07 |
| `std::ranlux24_base` | 4.01 |
| `std::ranlux48_base` | 3.09 |
| `std::ranlux24` | 37.29 |
| `std::ranlux48` | 104.72 |
| `std::knuth_b` | 6.41 |
| `std::mt19937` | 1.36 |
| `WFLCG` | 0.57 |
| `WFLCG` (direct) | 0.29 |

Running the benchmark on an iMac (i5-2400S) using clang (Apple LLVM version 10.0.0
clang-1000.11.45.5) with `-Ofast -march=native` yields the following results:

| Random number generator | Total time (seconds) |
|:-----------------------:|:--------------------:|
| `std::minstd_rand0` | 6.41 |
| `std::minstd_rand` | 5.17 |
| `std::ranlux24_base` | 6.81 |
| `std::ranlux48_base` | 7.27 |
| `std::ranlux24` | 74.02 |
| `std::ranlux48` | 264.28 |
| `std::knuth_b` | 7.11 |
| `std::mt19937` | 6.57 |
| `WFLCG` | 1.64 |
| `WFLCG` (direct) | 0.43 |

The last result comes from accessing the internal buffer of the class directly.
See below for details.

The speed of the class relies on the auto-vectorization optimizations of compilers
like gcc and clang. These speeds are thus heavily relying on turning on compiler
optimizations (eg. `-Ofast`).

## Basic usage

As an RNG meeting the requirements of _UniformRandomBitGenerator,_ it can be used in
the same way as any of the standard RNGs in `<random>`. For example:

```c++
#include "WFLCG.hh"

void foo()
{
    WFLCG rng;
    unsigned value = rng();
}
```

The class has a constructors taking zero, one or two initial seed values of type
`std::uint32_t`. The default initial seed value is `0`.

```c++
WFLCG rng1(100); // initialize with a seed of 100
WFLCG rng2(12, 34); // initialize with seeds 12 and 34
```

## Generating random floating point values

Member functions:

```c++
float getFloat();
double getDouble();
double getDouble2();
```

These can be used to get floating point values instead of integer values. They, too, are
designed to be as efficient as possible. Note, however, that they return values in the
range [1.0, 2.0) (because doing this requires merely a couple of integer bitwise operators).
If you want the value in the range [0.0, 1.0), subtract 1.0 from the returned value.

```c++
WFLCG rng;
float randFloat = rng.getFloat() - 1.0f;
```

The header also defines two additional convenience classes that meet the requirements of
_UniformRandomBitGenerator_ which return floats and doubles instead of integers. Note, however,
that these, too, return values in the range [1.0, 2.0).

```c++
class WFLCG_f; // returns random floats in the range [1.0, 2.0)
class WFLCG_d; // returns random doubles in the range [1.0, 2.0)
class WFLCG_d2; // returns random doubles in the range [1.0, 2.0)
```

Note that in the case of `getDouble()` / `WFLCG_d`, only the 32 most-significant bits of
the mantissa will be randomized. (The remaining 20 least-significant bits of the mantissa
will be zero.) This means that the returned double can have 2<sup>32</sup> different values.

`getDouble2()` and `WFLCG_d2` are versions of this that will consume two values from the
random number stream in order to fill the entire mantissa of the result (with high-quality
mixing). Thus the period of this version is 2<sup>35</sup>.

Important: Do not mix calls to `getDouble2()` with calls to any of the other value retrieval
functions, as (for performance reasons) no sanity checks are done to the internal index value.

## Direct value access

Even more efficiency can be achieved by reading the internal buffer of the class directly
(as seen in the benchmark results above). Accessing the buffer directly bypasses conditionals
and gives the compiler more optimization opportunities.

Member functions:

```c++
static constexpr unsigned kBufferSize = 16;

const std::uint32_t* buffer() const;

float bufferElementAsFloat(unsigned);
double bufferElementAsDouble(unsigned);
double bufferElementAsDouble2(unsigned);

void refillBuffer();
```

The values can be accessed very efficiently for example like this:

```c++
for(unsigned outer = 0; outer < someAmount; ++outer)
{
    for(unsigned inner = 0; inner < WFLCG::kBufferSize; ++inner)
        doSomethingWithValue(rng.buffer()[inner]);

    rng.refillBuffer();
}
```

Note, however, that the values inside the buffer are not of equal quality as the ones
returned by `WFLCG::operator()`. In some situations this might be enough. In order to
get the same values as the operator returns, the highest byte should be xorred with the
lowest, like:

```c++
    unsigned randValue = rng.buffer()[inner];
    randValue ^= randValue >> 24;
```

To get the elements of the buffer as a `float` or as a `double`, use the `bufferElementAsFloat()`
and `bufferElementAsDouble()` functions. (The latter does the xor operation described above for
increased randomness quality. The float version does not need to do this because the lower bits
are dropped in the mantissa anyway.) Note that no boundary checks are done to the parameter
given to these functions, which should be between 0 and `kBufferSize-1`. A parameter value
larger than that will cause an out-of-bounds access.

`bufferElementAsDouble2()` will take the value at the specified index as well as the one after
it. Therefore the parameter should be in the range between 0 and `kBufferSize-2`. The best
way to access this is to make the index jump by 2, like:

```c++
for(unsigned outer = 0; outer < someAmount; ++outer)
{
    for(unsigned inner = 0; inner < WFLCG::kBufferSize; inner += 2)
        doSomethingWithValue(rng.bufferElementAsDouble2(inner));

    rng.refillBuffer();
}
```

## C version

Types and functions for basic usage. Their functionality is equivalent to those of the C++
version.

```c
#define WFLCG_C_BUFFER_SIZE 16

typedef struct
{
    uint32_t buffer[WFLCG_C_BUFFER_SIZE];
    unsigned index;
} WFLCG_c;

void WFLCG_c_init_default(WFLCG_c* obj);
void WFLCG_c_init_1_seed(WFLCG_c* obj, uint32_t seed);
void WFLCG_c_init_2_seeds(WFLCG_c* obj, uint32_t seed1, uint32_t seed2)

uint32_t WFLCG_c_get_value(WFLCG_c* obj);
float WFLCG_c_get_float(WFLCG_c* obj);
double WFLCG_c_get_double(WFLCG_c* obj);
double WFLCG_c_get_double2(WFLCG_c* obj);
```

The buffer can be accessed directly in the struct (same notes apply as in the C++
version). The following functions are provided for direct access:

```c
void WFLCG_c_refill_buffer(WFLCG_c* obj);
float WFLCG_c_buffer_element_float(WFLCG_c* obj, unsigned index);
double WFLCG_c_buffer_element_double(WFLCG_c* obj, unsigned index);
double WFLCG_c_buffer_element_double2(WFLCG_c* obj, unsigned index);
```

Example of basic usage:

```c
#include "WFLCG_c.h"
#include <stdio.h>

int main()
{
    WFLCG_c rng;
    WFLCG_c_init_default(&rng);

    for(unsigned i = 0; i < 10; ++i)
    {
        uint32_t value = WFLCG_c_get_value(&rng);
        printf("%u\n", value);
    }
}
```

Example of direct access:

```c
#include "WFLCG_c.h"
#include <stdio.h>

int main()
{
    WFLCG_c rng;
    WFLCG_c_init_1_seed(&rng, 1234567);

    for(unsigned outer = 0; outer < 10; ++outer)
    {
        for(unsigned inner = 0; inner < WFLCG_C_BUFFER_SIZE; ++inner)
        {
            double value = WFLCG_c_buffer_element_double(&rng, inner);
            printf("%.15f\n", value);
        }

        WFLCG_c_refill_buffer(&rng);
    }
}
```

## FAQ

"Why not use 64-bit seeds, thus returning 64-bit values and getting a period of 2<sup>68</sup>?"

The speed of the class relies on auto-vectorization performed by modern compilers. This means
that the 16 multiplications and 16 additions done each time the internal buffer is repopulated
are performed by just 2 multiplication and 2 addition SSE/AVX opcodes (in other words,
8 multiplications are done at the same time with one single opcode, etc.)

The problem is that neither SSE nor AVX support doing this with 64-bit integers, only with
32-bit ones. (The first architecture supporting this with 64-bit integers is AVX-512, which
is still extremely uncommon, even in the most modern desktop CPUs, as of writing this.)
When using 64-bit integers, the compiler will generate additional code to get around this
limitation, which is slower than using 32-bit integers.
