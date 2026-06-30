#include "../../include/strategies/donChannel.hpp"
#include <algorithm>
#include <iostream>
#include <cmath>

donChannel::donChannel(Broker& b, Account& u, Bar& cB, std::unordered_map<long int, Trade>& history, std::string symbol) : Strategy(b, u, cB, history, symbol){
}


void donChannel::init(){
    std::cout << "Created a Donchian Channel(Breakout) Strategy" << std::endl;
}

void donChannel::runBar(){
    
    double currPrice = connectBar.close;
    

    Window.push_back(currPrice);

    
    

    //if both windows are filled then execute breakout logic
    if(Window.size() == windowSize){

        //std::minmax_element returns a pointer to the lowest and highest value
        auto [lowest, highest] = std::minmax_element(Window.begin(), Window.end());

        //delete order
        if(highestID != -1){
            broker.deleteOrder(highestID, "STRATEGY CREATING NEW ORDER");
            broker.deleteOrder(lowestID, "STRATEGY CREATING NEW ORDER");
        }
        
        double currBalance = user.checkBalance();
        //calculate 20% of currBalance worth in shares(floored in order to have int shares)
        long numShares = std::floor((currBalance*0.2)/currPrice);
        //create Order struct
        highestOrder = Order{ticker, "stop", 0, numShares, (*highest+0.01)};
        
        numShares = std::floor(user.positionQuantity(ticker)*0.2);
        lowestOrder = Order{ticker, "stop", 1, numShares, (*lowest-0.01)};

        highestID = broker.createOrder(highestOrder);
        lowestID = broker.createOrder(lowestOrder);
        

    }
    
}