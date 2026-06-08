#include "../include/createStrat.hpp"
#include "../include/strategies/smaCross.hpp"


static std::unique_ptr<Strategy> StrategyFactory::create(Broker& b, Account& u, Bar& cB, std::unordered_map<long int, Trade>& history, std::string symbol, std::string typeStrat){
    //check for which strat and include necessary parameters

    if(typeStrat == "sma"){
        //for now we will only use the default constructor
        return std::make_unique<smaCross>(b,u,cB,history,symbol);
    }

    return nullptr;
}
