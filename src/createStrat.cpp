#include "../include/createStrat.hpp"
#include "../include/strategies/smaCross.hpp"
#include "../include/strategies/bollBand.hpp"
#include "../include/strategies/donChannel.hpp"
#include <stdexcept>


std::unique_ptr<Strategy> StrategyFactory::create(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::vector<std::string> symbols, std::string typeStrat, const json& config){//the config is the paramaters for the specific strat in the json file

    //config["key"] throws if "key" isn't present in the json object -
    //config.value("key", fallback) is the safer alternative used here,
    //returning fallback instead of throwing when the key is missing. that
    //means older batch config files that don't mention position_size_pct
    //at all still work, they just get the same 0.2 default every strategy
    //always used before this was configurable
    //defaults to 0.2 (20%) if the config doesn't specify one, matching every
    //strategy's original hardcoded behavior
    double posSizePct = config.value("position_size_pct", 0.2);

    if(typeStrat == "sma"){
        //for now we will only use the default constructor
        //config["fast_period"].get<int>() reads the json value stored
        //under the key "fast_period" and converts it into a real c++ int -
        //without .get<int>() you'd just have a generic json value, not a
        //usable number
        int fast = config["fast_period"].get<int>();
        int slow = config["slow_period"].get<int>();
        //std::make_unique<smaCross>(...) constructs a new smaCross object
        //on the heap and immediately wraps it in a unique_ptr<Strategy> -
        //this is the standard, safer way to create a unique_ptr(rather
        //than writing "new smaCross(...)" yourself and wrapping it after)
        return std::make_unique<smaCross>(b,u,cBs,history,symbols[0], fast, slow, posSizePct);
    }
    else if(typeStrat == "boll"){
        int window = config["window"].get<int>();
        return std::make_unique<bollBand>(b,u,cBs,history,symbols[0], window, posSizePct);
    }
    else if(typeStrat == "don"){
        int window = config["window"].get<int>();
        return std::make_unique<donChannel>(b,u,cBs,history,symbols[0], window, posSizePct);
    }

    //throw hands control back up the call stack to whoever is prepared to
    //catch this specific exception type(std::invalid_argument) - since
    //nothing in main.cpp currently catches it, an unrecognized strategy
    //name will still end the program, but with a clear message printed
    //first, right at the point the actual mistake was made, instead of
    //crashing confusingly somewhere else later
    throw std::invalid_argument("Unknown strategy type: " + typeStrat);
}
