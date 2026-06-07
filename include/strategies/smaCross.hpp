#pragma once
#include "../strategy.hpp"
#include <queue>

class smaCross : public Strategy{
    public:
        void runBar();
    private:
        //size_t does not use negative numbers so we have a larger degree of numbers
        size_t slowLength;
        size_t fastLength;

        double fastSum;
        double slowSum;
        std::queue<double> fastWindow;
        std::queue<double> slowWindow;

        
        
};