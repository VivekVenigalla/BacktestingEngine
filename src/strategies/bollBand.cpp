#include "../../include/strategies/bollBand.hpp"
#include <cmath>
#include <iostream>


bollBand::bollBand(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::string symbol) : Strategy(b, u, cBs, history, symbol){
}
bollBand::bollBand(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::string symbol, int window) : Strategy(b, u, cBs, history, symbol), windowSize(window){
}
//delegates to the constructor above(see smaCross.cpp for what constructor
//delegation means) instead of repeating it, then only sets the one extra field
bollBand::bollBand(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::string symbol, int window, double posSizePct) : bollBand(b, u, cBs, history, symbol, window){
    positionSizePct = posSizePct;
}

void bollBand::init(){
    std::cout << "Created a Bollinger Band(Mean Reversion) Strategy" << std::endl;
}

void bollBand::runBar(){

    //add value to both sums and chekc if queues are filled(connectBar will have the most recent Bar)
    double currPrice = connectBars.begin()->second.close;

    windowSum += currPrice;

    Window.push(currPrice);

    //check if the queues are filled up and pop if necessary
    //same incremental-running-sum trick as smaCross: add the new price,
    //subtract the one that just fell out of the window, so windowSum stays
    //correct without re-adding every price in the window each bar
    if(Window.size() > windowSize){
        windowSum -= Window.front();
        Window.pop();
    }

    //if both windows are filled then execute bollinger band logic
    if(Window.size() == windowSize){
        //calculuate averages
        double windowAverage = windowSum/windowSize;

        //calculate standard deviation
        double stdDev = standardDeviation(Window, windowAverage);

        //create upper and lower bounds
        double upperBound = windowAverage + stdDev*2;
        double lowerBound = windowAverage - stdDev*2;


        if(currPrice < upperBound && state == 1){
            if(currPrice > windowAverage){
                long numShares = sizeSellOrder(user.positionQuantity(ticker));
                nextOrder = {ticker, "market", 1, numShares, -1.0};
                broker.createOrder(nextOrder);
            }
            state = 0;
        }
        else if(currPrice > lowerBound && state == -1){
            if(currPrice < windowAverage){
                //size the buy using the shared, configurable position-sizing helper
                long numShares = sizeBuyOrder(currPrice);
                //create Order struct
                nextOrder = {ticker, "market", 0, numShares, -1.0};
                broker.createOrder(nextOrder);
            }
            state = 0;
        }

        if(currPrice > upperBound && state == 0){
            state = 1;
        }
        else if(currPrice < lowerBound && state == 0){
            state = -1;
        }

    }

}

//standard deviation measures how spread out a set of numbers is from its
//average - a small value means prices have been sticking close to the
//average, a large value means they've been swinging widely. this is what
//makes the bands widen/narrow with volatility instead of sitting at a fixed
//distance from the average
double bollBand::standardDeviation(const std::queue<double>& nums, double average){
    //since the queue cannot be easily iterated without destroying the original, we create a copy
    std::queue<double> numsCopy = nums;
    //iterate through the queue, finding the difference between the element and the mean
    double temp = 0.0;
    while(!numsCopy.empty()){
        //std::pow(x, 2) squares x - here it turns each price's distance
        //from the average into a positive number(so distances above and
        //below the average don't cancel out when added together)
        temp += std::pow((numsCopy.front()-average), 2);
        numsCopy.pop();
    }
    //dividing by (windowSize-1) instead of windowSize is called bessel's
    //correction - it's the standard adjustment used when you're estimating
    //the spread of a whole population from just a sample of it(a fixed
    //rolling window here, not the entire real price history). std::sqrt
    //undoes the squaring from std::pow above, bringing the units back to
    //"price", not "price squared"
    temp = std::sqrt(temp/(windowSize-1));
    return temp;
}
