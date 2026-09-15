#include "monteCarlo.hpp"

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
