#pragma once

#include "./structures.hpp"
#include "./broker.hpp"
#include "./account.hpp"
#include <unordered_map>
#include <vector>

//the strategy contains the 

class Strategy{
    public:
        Strategy(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::string symbol);

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
        std::unordered_map<std::string, Bar>& connectBars;
        std::string ticker;

        //fraction of balance/quantity used per sizing call; 0.2 (20%) matches
        //what every concrete strategy hardcoded before this was extracted
        double positionSizePct = 0.2;
        long sizeBuyOrder(double price) const;
        long sizeSellOrder(long currentQuantity) const;

        //Places a protective stop-sell (entryPrice*(1-stopLossPct)) and a
        //protective limit-sell (entryPrice*(1+takeProfitPct)) after opening a
        //long position. NOT a true OCO (one-cancels-other) bracket: if one
        //fills, the other is left pending in the broker's order map and
        //simply fails checkOrder's validation (insufficient shares) on a
        //later bar rather than being cancelled -- it degrades gracefully
        //rather than crashing, but isn't cleaned up either. True OCO would
        //need order-linking at the Broker level, which is out of scope here.
        void placeBracketOrders(double entryPrice, long quantity, double stopLossPct, double takeProfitPct);

};