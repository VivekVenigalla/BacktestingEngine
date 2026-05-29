#include "../include/broker.hpp"
#include <iostream>


//WORK IN PROGRESS
//FIX create order and process order, while building method and means for obtaining the data values.

Broker::Broker(Account& account) : user(account){
    //no additional construction needed for now, since user is already referenced to account
}

int Broker::createOrder(Order newOrder){
    //the Broker must check if the order is a market, so that it will go into the market category
    if(newOrder.type == "market"){
        if(checkMarketOrder(newOrder)){
            marketorders[tempID] = newOrder;
            processOrder(newOrder.id);
        }
    }
    orders[tempID] = newOrder;
    ++tempID;
    return tempID-1;
    
}

void Broker::check(){
    //.begin and .end provide the iterators for the loop so we can iterate through the unordered_map
    for(auto it = orders.begin();it!=orders.end();){
        //since it is a iterator, in order to obtain the id, we use it->first
        if(checkOrder(it->first)){
            //.erase returns a empty iterator temporarily so we prevent index invalidation
            std::cout<<"Order(ID) " << it->first << " is now valid for processing. Processing...";
            processOrder(it->first);
            std::cout<<"Order(ID) " << it->first << "processed!";
            it = orders.erase(it);
        }
        else{
            //only increments if there was no deletion so we don't skip an order
            ++it;
        }
    }
}

bool Broker::checkMarketOrder(Order check){
    //check if the account has sufficient funds or if they have enough shares

    //buy
    if(check.side == 0){
        double tempBalance = user.checkBalance();
        //obtain the price(low) and tempBalance >= price*quantity
        return true;
        //remove return once function built
        
    }
    else{
        long tempShares = user.positionQuantity(check.ticker);
        if(check.quantity >= tempShares){
            return false;
        }
        else{
            return true;
        }
    }
}

bool Broker::checkOtherOrder(int id){
    //logic for checing if the order meets standards. Needs access to data however, so once that is up and running will implement

    //first check the type of order
    //if market order, continue on
    //if a limit or stop order, check the price
    //  limit: buy only if Bar's low is smaller than check_price and sell if Bar's high is larger than check_price
    //  stop : vice versa
    //then check if either the account has sufficient funds or if they have enough shares 
    return true;
}

void Broker::processOrder(int id){
    //also needs currPrice
    //creates the trade history struct and insters it into the history var
    
}

std::unordered_map<long int, Trade> Broker::returnHistory(){
    return history;
}
std::unordered_map<long int, Order> Broker::returnOrders(){
    return orders;
}