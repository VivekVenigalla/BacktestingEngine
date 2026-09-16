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

        //permutes(shuffles) the SAME set of real returns into a different
        //order, without replacement(every return is used exactly once per
        //run, just reordered) - unlike bootstrapResample, this can never
        //change the FINAL equity(multiplying the same numbers together in
        //a different order always gives the same product), so it answers a
        //different question: "how much could this exact set of wins/losses
        //have hurt ALONG THE WAY, if they'd landed in a worse sequence?".
        //two runs with identical final equity can have wildly different max
        //drawdowns depending purely on order(e.g all the losses landing
        //back-to-back near the start, vs spread out) - this returns the
        //worst percentage drawdown seen in each shuffled run
        std::vector<double> shuffledOrderResample(int numRuns, double initialEquity);

        //sorts a copy of values and returns the p-th percentile(0-100) using
        //linear interpolation between the two nearest ranks(the same method
        //numpy's default percentile uses) - more precise than just snapping
        //to the nearest existing data point when p doesn't land exactly on
        //an index. static because it needs no Monte Carlo state(returns/
        //rng) - it's a general-purpose stats helper, usable on either
        //bootstrapResample's or shuffledOrderResample's output
        static double percentile(std::vector<double> values, double p);

    private:
        std::vector<double> returns;
        //std::mt19937 is a fairly large object(a few KB of internal state) -
        //storing ONE as a member and reusing it across every call keeps
        //advancing the same random sequence, rather than restarting from
        //the same seed every time bootstrapResample is called
        std::mt19937 rng;
};
