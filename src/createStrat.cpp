#include "../include/createStrat.hpp"
#include "../include/strategies/smaCross.hpp"
#include "../include/strategies/bollBand.hpp"
#include "../include/strategies/donChannel.hpp"


std::unique_ptr<Strategy> StrategyFactory::create(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::vector<std::string> symbols, std::string typeStrat, const json& config){//the config is the paramaters for the specific strat in the json file
    
    //check if the cBs is only one in size
    /*if(cBs.size() == 1){
        //get the cB value as just a Bar type
        Bar& cB = cBs.begin()->second;
        //check for which strat and include necessary parameters

        if(typeStrat == "sma"){
            //for now we will only use the default constructor
            return std::make_unique<smaCross>(b,u,cB,history,symbol);
        }
        else if(typeStrat == "boll"){
            return std::make_unique<bollBand>(b,u,cB,history,symbol);
        }
        else if(typeStrat == "don"){
            return std::make_unique<donChannel>(b,u,cB,history,symbol);
        }

        return nullptr;

    }
    else{
        //revise once other strategies implemented
        return nullptr;
    }*/
    if(typeStrat == "sma"){
        //for now we will only use the default constructor
        int fast = config["fast_period"].get<int>(); 
        int slow = config["slow_period"].get<int>(); 
        return std::make_unique<smaCross>(b,u,cBs,history,symbols[0], fast, slow);
    }
    else if(typeStrat == "boll"){
        return std::make_unique<bollBand>(b,u,cBs,history,symbols[0]);
    }
    else if(typeStrat == "don"){
        return std::make_unique<donChannel>(b,u,cBs,history,symbols[0]);
    }

    return nullptr;
}
