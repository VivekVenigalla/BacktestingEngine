#pragma once

#include "./structures.hpp"
#include "./broker.hpp"
#include "./account.hpp"
#include <unordered_map>
#include <vector>

//the strategy contains the 

class Strategy{
    public:
        Strategy(Broker& b, Account& u, Bar& cB, std::unordered_map<long int, Trade>& history, std::string symbol);

        //this method lets the strategy know that the Bar reference has been changed and it can append it to its history
        void loadBar();
        virtual ~Strategy() = default;
        virtual void runBar() = 0;
        virtual void init() = 0;
        
    protected:
        //protected allows the following member fields to be accessed inside the class and inherited classes
        //the strategy class should be able to access a history of bars that were fed, and a history or trades
        std::vector<Bar> barHistory;
        std::unordered_map<long int, Trade>& tradeHistory;
        
        Broker& broker;
        Account& user;
        Bar& connectBar;
        std::string ticker;


};