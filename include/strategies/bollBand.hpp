#pragma once
#include "../strategy.hpp"
#include <queue>



//NOTES ABOUT STRATEGY
//This strategy uses Bollinger Bands, which is an example of mean reversion
//implements a sma of 20 periods
//The average has a window of two standard deviations
//If the price goes below the lower bound, the price is undersold
//when it comes back up buy in anticipation that the price will rise up to the middle
//If the price goes above the upper bound, the price is oversold
//when it comes back down sell in anticipation that the price will drop down to the middle

class bollBand : public Strategy{
    public:
        bollBand(Broker& b, Account& u, Bar& cB, std::unordered_map<long int, Trade>& history, std::string symbol);
        
        void runBar();
        void init();
        virtual ~bollBand() = default;
    private:
        //bollinger bands implement a 20 period windows, hence this is a const
        int windowSize = 20;
        //this state tells us where the price is
        //if the state is 0, price is between upper and lower bound
        //if the state is 1, price is above upper bound
        //if the state is -1, price is below lower bound
        int state = 0;
        double windowSum;
        Order nextOrder;
        //check is not currently used right now since the Broker::checkLoop() is executed on the next day now
        bool check = false;
        //implementing a queue allows for easy plug in and extraction so we dont have to iterate a lot.
        std::queue<double> Window;

        double standardDeviation(std::queue<double> nums, double average);

};