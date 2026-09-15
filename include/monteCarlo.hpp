#pragma once
#include <vector>
#include <random>

//a single backtest run only tells you what happened along ONE historical
//price path - the exact sequence of ups and downs that actually occurred.
//Monte Carlo asks a different question: "if the same set of trade outcomes
//had happened in a different order(or with some outcomes repeated/skipped),
//how differently could this have gone?" it does this WITHOUT re-running the
//bar-by-bar simulation again(that would be slow and pointless, since the
//underlying price data never changes) - instead it resamples the REAL
//period returns a completed run already produced, many times over, and
//looks at the spread of possible outcomes that resampling produces
class MonteCarloSimulator{
    public:
        //returns: a period-return series(e.g Metrics::returnsFromEquityCurve's
        //output) from one already-completed real backtest. a fractional
        //return of 0.05 means "+5% that period", matching the convention
        //returnsFromEquityCurve/sharpeRatio/sortinoRatio already use
        //
        //seed: std::mt19937 is a pseudo-random number generator - given the
        //SAME seed, it always produces the exact same sequence of "random"
        //numbers, which is what makes a Monte Carlo run reproducible(two
        //people running this with seed=42 get identical results, useful for
        //debugging and for tests). defaulting to std::random_device{}()
        //pulls a genuinely unpredictable seed from the OS when the caller
        //doesn't care about reproducibility and just wants fresh randomness
        //each time
        MonteCarloSimulator(std::vector<double> returns, unsigned int seed = std::random_device{}());

        //draws returns.size() samples PER RUN, WITH replacement(the same
        //historical return can be picked more than once, and some returns
        //might not get picked at all), compounds them starting from
        //initialEquity, and records where equity ended up. repeating this
        //numRuns times builds a distribution of "what if the same set of
        //trade outcomes had landed in a different order/mix" final-equity
        //values - this is the classic bootstrap resampling technique
        std::vector<double> bootstrapResample(int numRuns, double initialEquity);

    private:
        std::vector<double> returns;
        //std::mt19937 is a fairly large object(a few KB of internal state) -
        //storing ONE as a member and reusing it across every call keeps
        //advancing the same random sequence, rather than restarting from
        //the same seed every time bootstrapResample is called
        std::mt19937 rng;
};
