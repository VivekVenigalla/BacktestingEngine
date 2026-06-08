#pragma once
#include "../strategy.hpp"
#include <queue>


//NOTES ABOUT STRATEGY
//This strategy is called a simple moving average
//implements two windows, one fast and one slow(with the slow window a larger timeframe)
//when the fast crosses above the slow, buy 20% of current balance in shares(golden cross)
//vice versa if fast crosses below the slow, sell all shares(death cross)
//since the fast and slow window will be the same for the first 50 values, there should be no trades occuring then
//uses market close prices
//all orders are market
//since market orders are executed at the open price, all orders are executed the day after the order us creared to prevent look ahead bias

class smaCross : public Strategy{
    public:
        smaCross(Broker& b, Account& u, Bar& cB, std::unordered_map<long int, Trade>& history, std::string symbol);
        smaCross(Broker& b, Account& u, Bar& cB, std::unordered_map<long int, Trade>& history, std::string symbol, int fast, int slow);
        void runBar();
        void init();
        virtual ~smaCross() = default;
    private:
        int slowLength = 200;
        int fastLength = 50;

        double fastSum;
        double slowSum;
        Order nextOrder;
        bool check = false;
        //implementing a queue allows for easy plug in and extraction so we dont have to iterate a lot.
        std::queue<double> fastWindow;
        std::queue<double> slowWindow;

        
        
};