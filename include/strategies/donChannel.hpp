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
        
        void runBar();
        void init();
        virtual ~donChannel() = default;
    private:
        //this breakout strategy implement a 20 period windows
        int windowSize = 20;

        //pointer to the highest and lowest value
        double* highest;
        double* lowest;

        //int id values for the order created
        int highestID = -1;
        int lowestID;

        Order highestOrder;
        Order lowestOrder;
        bool check = false;
        //implementing a queue allows for easy plug in and extraction so we dont have to iterate a lot.
        std::vector<double> Window;

};