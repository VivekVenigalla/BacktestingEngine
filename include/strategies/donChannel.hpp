#pragma once
#include "../strategy.hpp"
#include <queue>



//NOTES ABOUT STRATEGY
//This strategy is a version of a breakout strategy
//Over a 20 period window the strategy finds the highest and lowest value
//The strategy creates a stop buy order 1 cent over the highest value and stop loss 1 cent less below the lowest value

class donChannel : public Strategy{
    public:
        donChannel(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::string symbol);
        donChannel(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::string symbol, int window);
        donChannel(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::string symbol, int window, double posSizePct);
        void runBar();
        void init();
        virtual ~donChannel() = default;
    private:
        //this breakout strategy implement a 20 period windows
        int windowSize = 20;

        //heads up: these two are never actually used - runBar() computes
        //the window's highest/lowest with LOCAL variables of the exact
        //same names(see the "auto [lowest, highest] = ..." line in the
        //.cpp), which shadow(hide) these member fields for the rest of
        //that block. these pointers are always left null and never
        //allocated - leftover from an earlier version of this strategy
        //pointer to the highest and lowest value
        double* highest;
        double* lowest;

        //int id values for the order created
        int highestID = -1;
        int lowestID;

        Order highestOrder;
        Order lowestOrder;
        bool check = false;
        //a vector here instead of a queue(unlike smaCross/bollBand's
        //windows) because this strategy needs to scan the WHOLE window for
        //its min/max every bar(see std::minmax_element in the .cpp), not
        //just add/remove from the ends - a queue doesn't support that kind
        //of full scan as conveniently as a vector does
        //implementing a queue allows for easy plug in and extraction so we dont have to iterate a lot.
        std::vector<double> Window;

};
