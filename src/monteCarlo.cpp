#include "monteCarlo.hpp"
#include <algorithm>
#include <cmath>

//": returns(returns), rng(seed)" is a member initializer list - rng(seed)
//specifically is how you construct a std::mt19937 seeded with a particular
//starting value(the same syntax as calling std::mt19937(seed) directly,
//just written as a member initializer instead)
MonteCarloSimulator::MonteCarloSimulator(std::vector<double> returns, unsigned int seed) : returns(returns), rng(seed){
}

std::vector<double> MonteCarloSimulator::bootstrapResample(int numRuns, double initialEquity){
    std::vector<double> finalEquities;
    finalEquities.reserve(numRuns);

    //nothing to resample from(e.g a strategy that never traded) - every
    //run just "stays at" the starting equity, since there's no return data
    //to draw from
    if(returns.empty()){
        finalEquities.assign(numRuns, initialEquity);
        return finalEquities;
    }

    //std::uniform_int_distribution picks a whole number uniformly at
    //random between its two bounds(both INCLUSIVE) - here, a valid index
    //into the returns vector. rng is what actually supplies the underlying
    //randomness; the distribution just shapes that randomness into "an
    //index between 0 and returns.size()-1" instead of some other range
    std::uniform_int_distribution<size_t> indexPicker(0, returns.size()-1);

    for(int run = 0; run < numRuns; run++){
        double equity = initialEquity;

        //draw returns.size() samples WITH replacement(indexPicker can hand
        //back the same index twice, or never hand back some index at all)
        //and compound them one after another - this is what "resampling"
        //actually means here: building a brand new, synthetic sequence of
        //returns out of the real ones, then seeing where compounding that
        //sequence would have left the account
        for(size_t i = 0; i < returns.size(); i++){
            size_t pickedIndex = indexPicker(rng);
            equity *= (1.0 + returns[pickedIndex]);
        }

        finalEquities.push_back(equity);
    }

    return finalEquities;
}

std::vector<double> MonteCarloSimulator::shuffledOrderResample(int numRuns, double initialEquity){
    std::vector<double> maxDrawdowns;
    maxDrawdowns.reserve(numRuns);

    //nothing to shuffle - every run just sits flat at the starting equity,
    //so there's no drawdown at all
    if(returns.empty()){
        maxDrawdowns.assign(numRuns, 0.0);
        return maxDrawdowns;
    }

    //a local, mutable copy to shuffle in place each run - the member
    //"returns" itself is never reordered, so every run starts from the
    //same original sequence before being shuffled fresh
    std::vector<double> shuffled = returns;

    for(int run = 0; run < numRuns; run++){
        //std::shuffle randomly reorders a range in place using rng to
        //decide the reordering - the standard library's replacement for
        //hand-rolling a Fisher-Yates shuffle yourself
        std::shuffle(shuffled.begin(), shuffled.end(), rng);

        double equity = initialEquity;
        double peak = initialEquity;
        double worstDrawdown = 0.0;

        for(double r : shuffled){
            equity *= (1.0 + r);
            if(equity > peak){
                peak = equity;
            }
            //same percentage-drawdown formula Metrics::drawDown/maxDrawdown
            //already use elsewhere in this codebase - always <=0, since
            //peak can never be smaller than the current equity by
            //definition
            double drawdown = (equity - peak) / peak * 100.0;
            if(drawdown < worstDrawdown){
                worstDrawdown = drawdown;
            }
        }

        maxDrawdowns.push_back(worstDrawdown);
    }

    return maxDrawdowns;
}

double MonteCarloSimulator::percentile(std::vector<double> values, double p){
    if(values.empty()){
        return 0.0;
    }

    std::sort(values.begin(), values.end());

    //rank is a FRACTIONAL index into the sorted array - e.g the 50th
    //percentile of a 5-element array gives rank=(50/100)*(5-1)=2.0,
    //landing exactly on index 2(the middle element). a percentile that
    //doesn't land on a whole index(like the 90th percentile of that same
    //5-element array, rank=3.6) gets linearly interpolated between the
    //values at index 3 and index 4 below, instead of just picking one
    double rank = (p / 100.0) * static_cast<double>(values.size() - 1);
    size_t lowerIndex = static_cast<size_t>(std::floor(rank));
    size_t upperIndex = static_cast<size_t>(std::ceil(rank));

    if(lowerIndex == upperIndex){
        return values[lowerIndex];
    }

    double fraction = rank - static_cast<double>(lowerIndex);
    return values[lowerIndex] + fraction * (values[upperIndex] - values[lowerIndex]);
}
