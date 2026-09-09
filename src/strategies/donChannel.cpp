#include "../../include/strategies/donChannel.hpp"
#include <algorithm>
#include <iostream>
#include <cmath>

donChannel::donChannel(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::string symbol) : Strategy(b, u, cBs, history, symbol){
}
donChannel::donChannel(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::string symbol, int window) : Strategy(b, u, cBs, history, symbol), windowSize(window){
}
donChannel::donChannel(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::string symbol, int window, double posSizePct) : donChannel(b, u, cBs, history, symbol, window){
    positionSizePct = posSizePct;
}

void donChannel::init(){
    std::cout << "Created a Donchian Channel(Breakout) Strategy" << std::endl;
}

void donChannel::runBar(){
    
    double currPrice = connectBars.begin()->second.close;
    
    //if both windows are filled then execute breakout logic
    if(Window.size() == windowSize){

        //std::minmax_element returns a pointer to the lowest and highest value
        auto [lowest, highest] = std::minmax_element(Window.begin(), Window.end());

        //delete order
        if(highestID != -1){
            broker.deleteOrder(highestID, "STRATEGY CREATING NEW ORDER");
            broker.deleteOrder(lowestID, "STRATEGY CREATING NEW ORDER");
        }
        
        //size both stop orders using the shared, configurable position-sizing helper
        long numShares = sizeBuyOrder(currPrice);
        //create Order struct
        highestOrder = Order{ticker, "stop", 0, numShares, (*highest+0.01)};

        numShares = sizeSellOrder(user.positionQuantity(ticker));
        lowestOrder = Order{ticker, "stop", 1, numShares, (*lowest-0.01)};

        highestID = broker.createOrder(highestOrder);
        lowestID = broker.createOrder(lowestOrder);
        

    }
    
    Window.push_back(currPrice);

    if(Window.size() > windowSize){
        Window.erase(Window.begin());
    }
    

    
}