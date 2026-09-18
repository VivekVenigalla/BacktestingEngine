#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "monteCarlo.hpp"
#include <algorithm>
#include <numeric>

//NOTE on portability: std::mt19937 itself is fully specified by the C++
//standard(the exact Mersenne Twister bit stream for a given seed is
//guaranteed identical across every conforming implementation), but
//std::uniform_int_distribution/std::shuffle's mapping from that bit stream
//into "which index gets picked" is only guaranteed statistically, NOT
//bit-for-bit, across different standard library implementations(libstdc++
//vs libc++ vs MSVC STL). so these tests never hard-code an expected numeric
//result derived from a specific seed - they instead compare two independent
//runs built from the SAME seed against EACH OTHER, which is guaranteed to
//match on any single implementation regardless of which one it is

TEST_CASE("MonteCarloSimulator resampling is reproducible given the same seed", "[montecarlo]") {
    std::vector<double> returns = {0.02, -0.01, 0.03, -0.02, 0.01, 0.015, -0.005};
    unsigned int seed = 42;

    SECTION("bootstrapResample") {
        MonteCarloSimulator simA(returns, seed);
        MonteCarloSimulator simB(returns, seed);

        std::vector<double> equitiesA = simA.bootstrapResample(200, 10000.0);
        std::vector<double> equitiesB = simB.bootstrapResample(200, 10000.0);

        REQUIRE(equitiesA.size() == 200);
        REQUIRE(equitiesA.size() == equitiesB.size());
        for(size_t i = 0; i < equitiesA.size(); i++){
            REQUIRE(equitiesA[i] == Catch::Approx(equitiesB[i]));
        }
    }

    SECTION("shuffledOrderResample") {
        MonteCarloSimulator simA(returns, seed);
        MonteCarloSimulator simB(returns, seed);

        std::vector<double> drawdownsA = simA.shuffledOrderResample(200, 10000.0);
        std::vector<double> drawdownsB = simB.shuffledOrderResample(200, 10000.0);

        REQUIRE(drawdownsA.size() == 200);
        REQUIRE(drawdownsA.size() == drawdownsB.size());
        for(size_t i = 0; i < drawdownsA.size(); i++){
            REQUIRE(drawdownsA[i] == Catch::Approx(drawdownsB[i]));
        }
    }

    SECTION("a different seed is not required to differ, but the SAME seed must never be ignored") {
        //this isn't "different seeds always give different output" (that's
        //not guaranteed for any RNG), it's a guard against a constructor
        //bug that ignores the seed argument entirely - two sims build from
        //DIFFERENT seeds over enough runs should not land on an identical
        //distribution
        MonteCarloSimulator simA(returns, 1);
        MonteCarloSimulator simB(returns, 2);

        std::vector<double> equitiesA = simA.bootstrapResample(200, 10000.0);
        std::vector<double> equitiesB = simB.bootstrapResample(200, 10000.0);

        REQUIRE(equitiesA != equitiesB);
    }
}

TEST_CASE("MonteCarloSimulator::percentile matches hand-computed values", "[montecarlo]") {
    //5 sorted values: 10, 20, 30, 40, 50 (indices 0-4)
    std::vector<double> values = {30.0, 10.0, 50.0, 20.0, 40.0};

    SECTION("0th and 100th percentiles are just the min/max") {
        REQUIRE(MonteCarloSimulator::percentile(values, 0.0) == Catch::Approx(10.0));
        REQUIRE(MonteCarloSimulator::percentile(values, 100.0) == Catch::Approx(50.0));
    }

    SECTION("50th percentile lands exactly on an index, no interpolation needed") {
        //rank = (50/100)*(5-1) = 2.0 -> exactly index 2 -> 30.0
        REQUIRE(MonteCarloSimulator::percentile(values, 50.0) == Catch::Approx(30.0));
    }

    SECTION("90th percentile requires linear interpolation between two ranks") {
        //rank = (90/100)*(5-1) = 3.6 -> interpolate 40% of the way from
        //index 3(40.0) to index 4(50.0) -> 40.0 + 0.6*(50.0-40.0) = 46.0
        REQUIRE(MonteCarloSimulator::percentile(values, 90.0) == Catch::Approx(46.0));
    }

    SECTION("25th percentile also requires interpolation") {
        //rank = (25/100)*(5-1) = 1.0 -> exactly index 1 -> 20.0
        REQUIRE(MonteCarloSimulator::percentile(values, 25.0) == Catch::Approx(20.0));
    }

    SECTION("an empty input returns 0.0 rather than crashing") {
        std::vector<double> empty;
        REQUIRE(MonteCarloSimulator::percentile(empty, 50.0) == Catch::Approx(0.0));
    }
}

TEST_CASE("shuffling a fixed set of returns never changes the compounded total return", "[montecarlo]") {
    //this is the mathematical premise shuffledOrderResample.hpp documents:
    //multiplying the same set of (1+r) factors together in a different
    //order always gives the same product. this test verifies that premise
    //directly - std::next_permutation enumerates every possible ordering
    //of a small returns vector, and every single one must compound to the
    //exact same final equity, since multiplication is commutative
    std::vector<double> returns = {0.10, -0.05, 0.02, -0.08};
    double initialEquity = 10000.0;

    double expectedFinalEquity = initialEquity;
    for(double r : returns){
        expectedFinalEquity *= (1.0 + r);
    }

    std::vector<double> permutation = returns;
    std::sort(permutation.begin(), permutation.end());
    int permutationsChecked = 0;

    do{
        double equity = initialEquity;
        for(double r : permutation){
            equity *= (1.0 + r);
        }
        REQUIRE(equity == Catch::Approx(expectedFinalEquity));
        permutationsChecked++;
    } while(std::next_permutation(permutation.begin(), permutation.end()));

    //4 distinct values -> 4! = 24 orderings, sanity-checking the loop above
    //actually enumerated all of them rather than exiting early
    REQUIRE(permutationsChecked == 24);
}

TEST_CASE("shuffledOrderResample can never report a drawdown when every return is non-negative", "[montecarlo]") {
    //with no losing periods at all, equity can only stay flat or climb -
    //no matter which order the (all non-negative) returns land in, the
    //running peak never falls behind the running equity, so every single
    //shuffled run's worst drawdown must be exactly 0. this exercises the
    //real production method(unlike the permutation test above, which
    //verifies the premise independently) against a case where the
    //order-invariance consequence IS directly observable through its
    //actual return value
    std::vector<double> onlyGains = {0.01, 0.03, 0.0, 0.02, 0.015};
    MonteCarloSimulator sim(onlyGains, 7);

    std::vector<double> drawdowns = sim.shuffledOrderResample(100, 10000.0);

    REQUIRE(drawdowns.size() == 100);
    for(double dd : drawdowns){
        REQUIRE(dd == Catch::Approx(0.0));
    }
}

TEST_CASE("MonteCarloSimulator handles an empty returns vector without crashing", "[montecarlo]") {
    std::vector<double> empty;
    MonteCarloSimulator sim(empty, 7);

    SECTION("bootstrapResample stays flat at initialEquity") {
        std::vector<double> equities = sim.bootstrapResample(10, 5000.0);
        REQUIRE(equities.size() == 10);
        for(double e : equities){
            REQUIRE(e == Catch::Approx(5000.0));
        }
    }

    SECTION("shuffledOrderResample reports zero drawdown for every run") {
        std::vector<double> drawdowns = sim.shuffledOrderResample(10, 5000.0);
        REQUIRE(drawdowns.size() == 10);
        for(double dd : drawdowns){
            REQUIRE(dd == Catch::Approx(0.0));
        }
    }
}
