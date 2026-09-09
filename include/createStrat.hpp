#pragma once
#include "./strategy.hpp"
#include "./structures.hpp"
#include "./broker.hpp"
#include "./account.hpp"
#include <unordered_map>
#include <memory>
#include "nlohmann/json.hpp"

//"using json = nlohmann::json;" is a type alias - it just gives a shorter
//nickname("json") to the long real name(nlohmann::json), the library this
//project uses to read/write json files(the batch config files under
//config/batchConfig/ are parsed into this type)
using json = nlohmann::json;

//StrategyFactory is the "factory" design pattern: instead of main.cpp
//needing to know how to construct a smaCross vs a bollBand vs a donChannel
//itself, it just says "give me a strategy of this type name" and this
//class handles picking/constructing the right concrete class
class StrategyFactory{
    public:
        //since this function does not need a object class it is static
        //static means this function belongs to the class itself, not to
        //any particular StrategyFactory object - that's why it's called as
        //StrategyFactory::create(...) rather than needing you to make a
        //StrategyFactory instance first(there'd be no point, since this
        //class has no fields of its own)
        //
        //std::unique_ptr<Strategy> is a smart pointer - it owns the
        //Strategy object it points to and automatically deletes it when
        //the unique_ptr itself goes out of scope, so nobody has to
        //remember to call "delete" by hand. "unique" means only one
        //unique_ptr can own that object at a time(ownership can be moved,
        //but never copied) - see make_unique<smaCross>(...) etc in the
        //.cpp for how one actually gets created
        static std::unique_ptr<Strategy> create(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::vector<std::string> symbols, std::string typeStrat, const json& config);
};
