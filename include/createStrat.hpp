#pragma once
#include "./strategy.hpp"
#include "./structures.hpp"
#include "./broker.hpp"
#include "./account.hpp"
#include <unordered_map>
#include <memory>

class StrategyFactory{
    public:
        //since this function does not need a object class it is static
        static std::unique_ptr<Strategy> create(Broker& b, Account& u, std::unordered_map<std::string, Bar> cBs, std::unordered_map<long int, Trade>& history, std::string symbol, std::string typeStrat);
};