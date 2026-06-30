#include "../../include/strategies/bollBand.hpp"
#include <cmath>
#include <iostream>


bollBand::bollBand(Broker& b, Account& u, Bar& cB, std::unordered_map<long int, Trade>& history, std::string symbol) : Strategy(b, u, cB, history, symbol){
}


void bollBand::init(){
    std::cout << "Created a Bollinger Band(Mean Reversion) Strategy" << std::endl;
}

void bollBand::runBar(){
    
    //add value to both sums and chekc if queues are filled(connectBar will have the most recent Bar)
    double currPrice = connectBar.close;
    
    windowSum += currPrice;

    Window.push(currPrice);
    
    //check if the queues are filled up and pop if necessary
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
                long numShares = std::floor(user.positionQuantity(ticker)*0.2);
                nextOrder = {ticker, "market", 1, numShares, -1.0};
                broker.createOrder(nextOrder);
            }
            state = 0;
        }
        else if(currPrice > lowerBound && state == -1){
            if(currPrice < windowAverage){
                double currBalance = user.checkBalance();
                //calculate 20% of currBalance worth in shares(floored in order to have int shares)
                long numShares = std::floor((currBalance*0.2)/currPrice);
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

double bollBand::standardDeviation(std::queue<double> nums, double average){
    //since the queue cannot be easily iterated without destroying the original, we create a copy
    std::queue<double> numsCopy = nums;
    //iterate through the queue, finding the difference between the element and the mean
    double temp;
    while(!numsCopy.empty()){
        temp += std::pow((numsCopy.front()-average), 2);
        numsCopy.pop();
    }
    temp = std::sqrt(temp/(windowSize-1));
    return temp;
}