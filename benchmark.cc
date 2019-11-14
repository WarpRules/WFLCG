/*
 std::minstd_rand0:   3.60 s
  std::minstd_rand:   3.07 s
std::ranlux24_base:   4.01 s
std::ranlux48_base:   3.09 s
     std::ranlux24:  37.29 s
     std::ranlux48: 104.72 s
      std::knuth_b:   6.41 s
      std::mt19937:   1.36 s
             WFLCG:   0.57 s
    WFLCG (direct):   0.29 s
*/

#include "WFLCG.hh"
#include <chrono>
#include <cstdio>
#include <random>

class Timer
{
    std::chrono::time_point<std::chrono::high_resolution_clock> mStartTime =
        std::chrono::high_resolution_clock::now();


 public:
    double getElapsedSeconds() const
    {
        const std::chrono::time_point<std::chrono::high_resolution_clock> endTime =
            std::chrono::high_resolution_clock::now();
        const std::chrono::duration<double> diff = endTime - mStartTime;
        return diff.count();
    }

    void printResult(const char* name)
    {
        const double seconds = getElapsedSeconds();
        std::printf("%18s: %.2f s\n", name, seconds);
    }
};

const std::size_t kIterations = 1000000000;
volatile unsigned gValueSink;

template<typename Rng_t>
void runBenchmark(const char* name)
{
    Rng_t rng(0);
    unsigned sum = 0;

    Timer timer;
    for(std::size_t i = 0; i < kIterations; ++i)
        sum += rng();

    gValueSink = sum;

    timer.printResult(name);
}

int main()
{
    runBenchmark<std::minstd_rand0>("std::minstd_rand0");
    runBenchmark<std::minstd_rand>("std::minstd_rand");
    runBenchmark<std::ranlux24_base>("std::ranlux24_base");
    runBenchmark<std::ranlux48_base>("std::ranlux48_base");
    //runBenchmark<std::ranlux24>("std::ranlux24");
    //runBenchmark<std::ranlux48>("std::ranlux48");
    runBenchmark<std::knuth_b>("std::knuth_b");
    runBenchmark<std::mt19937>("std::mt19937");
    runBenchmark<WFLCG>("WFLCG");

    WFLCG rng(0);
    const std::uint32_t* buffer = rng.buffer();
    unsigned sum = 0;

    Timer timer;

    const std::size_t iterations = kIterations / WFLCG::kBufferSize;
    for(std::size_t iteration = 0; iteration < iterations; ++iteration)
    {
        for(unsigned bufferInd = 0; bufferInd < WFLCG::kBufferSize; ++bufferInd)
            sum += buffer[bufferInd];
        rng.refillBuffer();
    }

    gValueSink = sum;

    timer.printResult("WFLCG (direct)");
}
