#include "../include/broker.hpp"
#include <iostream>


//WORK IN PROGRESS
//FIX create order and process order, while building method and means for obtaining the data values.

Broker::Broker(Account& account, std::unordered_map<std::string,Bar>& connectBar) : user(account),currBars(connectBar) {
    //no additional construction needed for now, since user and currBar is already referenced
}

Broker::Broker(Account& account, std::unordered_map<std::string,Bar>& connectBar, double commision, double slippage) : user(account),currBars(connectBar),commisionFee(commision),slippageRate(slippage){
    
}



//returns the ID of the order if it is exectable at request time. If not then returns 0
int Broker::createOrder(Order newOrder){
    //the Broker must check if the order is a market, so that it will go into the market category
    //there is no need to have a map for market orders since they are immediately executed once requested
    //orders are checled immediately to see if they are viable with the current funds/shares
    if(checkOrder(newOrder)){
        orders[tempID] = newOrder;
        /*if(newOrder.type == "market"){
            processOrder(tempID, newOrder);
        }
        else{
            orders[tempID] = newOrder;
        }*/
    }
    //null order if no availabel funds or shares
    else{
        Trade tempTrade;
        tempTrade.ticker = newOrder.ticker;
        tempTrade.execPrice = 0.0;
        tempTrade.type = newOrder.type;
        tempTrade.side = newOrder.quantity;
        tempTrade.quantity = newOrder.quantity;
        tempTrade.checkPrice = newOrder.checkPrice;
        tempTrade.filled = false;
        tempTrade.status = "ORDER " + std::to_string(tempID) + " FAILED TO FILL: LACK OF FUNDS OR SHARES OR ATTEMPT TO ORDER 0 SHARES";
        history[tempID] = tempTrade;
    }
    ++tempID;
    return tempID-1;
    
}

void Broker::deleteOrder(int orderID, std::string reason){
    //access order using the id
    Order newOrder = orders[orderID];
    Trade tempTrade;
    tempTrade.ticker = newOrder.ticker;
    tempTrade.execPrice = 0.0;
    tempTrade.type = newOrder.type;
    tempTrade.side = newOrder.quantity;
    tempTrade.quantity = newOrder.quantity;
    tempTrade.checkPrice = newOrder.checkPrice;
    tempTrade.filled = false;
    tempTrade.status = "ORDER " + std::to_string(tempID) + " CANCELLED : " + reason;
    //create trade history entry
    history[tempID] = tempTrade;
    //erase the order
    orders.erase(orderID);
}

//param:reference to the order to check to preserve memory and efficiency of not having to copy the order again
//return: bool=> true if executable at time requested and false if not
bool Broker::checkOrder(Order& check){
    //if the position quantity is 0, immediately return false
    if(check.quantity ==0){
        return false;
    }
    
    //check if the account has sufficient funds or if they have enough shares
    
    //buy
    if(check.side == 0){
        double tempBalance = user.checkBalance();
        double currPrice;
        //obtain the bar related to this ticker
        Bar& currBar = currBars[check.ticker];
        //obtain the price(low) and tempBalance >= price*quantity
        if(check.type == "market"){
            currPrice = currBar.open;
            
        }
        else if(check.type == "limit"){
            currPrice = check.checkPrice;
            
        }
        else{
            currPrice = currBar.close;
        
        }
        //add the commision fee
        currPrice += commisionFee;
        if(tempBalance >= currPrice*check.quantity){
            return true;
        }
        else{
            return false;
        }
        //remove return once function built
        
    }
    //sell
    else{
        long tempShares = user.positionQuantity(check.ticker);
        if(check.quantity > tempShares){
            return false;
        }
        else{
            return true;
        }

    }
}

void Broker::checkLoop(){
    //.begin and .end provide the iterators for the loop so we can iterate through the unordered_map
    for(auto it = orders.begin();it!=orders.end();){
        //since it is a iterator, in order to obtain the id, we use it->first

        //check if it is market and immediately process if so
        if(it->second.type == "market"){
            processOrder(it->first, it->second);
            it = orders.erase(it);
            continue;
        }
        if(checkOrderLimitAndStop(it->second)){
            if(checkOrder(it->second)){
                //.erase returns a empty iterator temporarily so we prevent index invalidation
                processOrder(it->first, it->second);
                it = orders.erase(it);
                continue;   
            }
            //passed required price but lack of funds or shares so order is nulled
            else{
                int id = it->first;
                Order order = it->second;
                Trade tempTrade;
                tempTrade.ticker = order.ticker;
                tempTrade.execPrice = 0.0;
                tempTrade.type = order.type;
                tempTrade.side = order.quantity;
                tempTrade.quantity = order.quantity;
                tempTrade.checkPrice = order.checkPrice;
                tempTrade.filled = false;
                tempTrade.status = "ORDER " + std::to_string(id) + " FAILED TO FILL: LACK OF FUNDS OR SHARES OR ATTEMPT TO ORDER 0 SHARES";
                std::cout << "ORDER STATUS: " << tempTrade.status << "\n";
                history[it->first] = tempTrade;
                it = orders.erase(it); 
                continue;
            }
        }
        else{
            //only increments if there was no deletion so we don't skip an order
            ++it;
        }
    }
}


bool Broker::checkOrderLimitAndStop(Order check){
    //logic for checing if the order meets standards. Needs access to data however, so once that is up and running will implement

    //first check the type of order
    //if market order, continue on
    //if a limit or stop order, check the price
    //  limit: buy only if Bar's low is smaller than check_price and sell if Bar's high is larger than check_price
    //  stop : vice versa
    //then check if either the account has sufficient funds or if they have enough shares 
    //these checks do not include the commision fee
    Bar& currBar = currBars[check.ticker];
    if(check.type == "limit"){
        if(check.side == 0){
            if(currBar.low <= check.checkPrice){
                return true;
            }else{
                return false;
            }
        }
        else{
            if(currBar.high >= check.checkPrice){
                return true;
            }else{
                return false;
            }
        }
    }
    else{
        if(check.side == 1){
            if(currBar.low <= check.checkPrice){
                return true;
            }else{
                return false;
            }
        }
        else{
            if(currBar.high >= check.checkPrice){
                return true;
            }else{
                return false;
            }
        }
    }
}

void Broker::processOrder(int id, Order order){
    //also needs currPrice
    //creates the trade history struct and insters it into the history var
    Bar& currBar = currBars[order.ticker];
    double currPrice;
    Trade tempTrade;

    //assign currPrice with either high or low depending on the order side and type

    //This implementation for the execPrice is shortsighted as we are using only 1 day intervals. change in the future
    if(order.type == "market"){
        if(order.side ==0){
            currPrice = (currBar.open)*(1.0+slippageRate)+commisionFee;  
        }
        else{
            currPrice = (currBar.open)*(1.0-slippageRate)-commisionFee; 
        }
        tempTrade.execPrice = currPrice;  
    }
    else if(order.type == "limit"){
        if(order.side ==0){
            currPrice = (order.checkPrice)*(1.0+slippageRate)+commisionFee;  
        }
        else{
            currPrice = (order.checkPrice)*(1.0-slippageRate)-commisionFee;   
        }
        tempTrade.execPrice = currPrice; 
    }
    else{
        if(order.side ==0){
            currPrice = (currBar.close)*(1.0+slippageRate)+commisionFee;  
        }
        else{
            currPrice = (currBar.close)*(1.0-slippageRate)-commisionFee; 
        }
    }

    //create trade histroy record
    tempTrade.ticker = order.ticker;
    tempTrade.execPrice = currPrice;
    tempTrade.type = order.type;
    tempTrade.side = order.side;
    tempTrade.quantity = order.quantity;
    tempTrade.checkPrice = order.checkPrice;
    tempTrade.commision = commisionFee;
    
    //check if position exists on user account or not and fill out order
    if(user.checkPosition(order.ticker)){
        if(order.side == 0){
            tempTrade.filled = true;
            tempTrade.status = "ORDER " + std::to_string(id) + " FILLED: BUY " + order.ticker + " " + std::to_string(order.quantity) + " FOR " + " " + std::to_string(currPrice);
            
            user.buyPositionQuantity(order.ticker, order.quantity, currPrice);
            
            tempTrade.currBalance = user.checkBalance();
            std::cout << "ORDER STATUS: " << tempTrade.status << "\n";
            std::cout << "CURRENT BALANCE: " <<  tempTrade.currBalance << "\n";
        }
        else{
            if(user.positionQuantity(order.ticker) == order.quantity){
                //sell all
                tempTrade.filled = true;
                tempTrade.status = "ORDER " + std::to_string(id) + " FILLED: SELL " + order.ticker + " ALL FOR " + " " + std::to_string(currPrice);
                
                user.sellAllPosition(order.ticker, currPrice);
                
                tempTrade.currBalance = user.checkBalance();
                std::cout << "ORDER STATUS: " << tempTrade.status << "\n";
                std::cout << "CURRENT BALANCE: " <<  tempTrade.currBalance << "\n";
                
            }
            else if(user.positionQuantity(order.ticker) > order.quantity){
                tempTrade.filled = true;
                tempTrade.status = "ORDER " + std::to_string(id) + " FILLED: SELL " + order.ticker + " " + std::to_string(order.quantity) + " FOR " + " " + std::to_string(currPrice);
                
                user.sellPositionQuantity(order.ticker, order.quantity, currPrice);
                
                tempTrade.currBalance = user.checkBalance();
                std::cout << "ORDER STATUS: " << tempTrade.status << "\n";
                std::cout << "CURRENT BALANCE: " <<  tempTrade.currBalance << "\n";
            }
            //should not occur due to logic but just in case
            else{
                return;
            }
        }
    }
    //if position does not exist(create new position or output error if selling)
    else{
        if(order.side == 1){
            tempTrade.filled = false;
            tempTrade.status = "ORDER " + std::to_string(id) + " FAILED TO FILL: ATTEMPT TO SELL POSITION THAT DOES NOT EXIST";
            std::cout << "ORDER STATUS: " << tempTrade.status << "\n";
        }
        else{
            tempTrade.filled = true;
            tempTrade.status = "ORDER " + std::to_string(id) + " FILLED: BUY " + order.ticker + " " + std::to_string(order.quantity) + " FOR " + " " + std::to_string(currPrice);
            
            user.buyNewPosition(order.ticker, order.quantity, currPrice);
            
            tempTrade.currBalance = user.checkBalance();
            std::cout << "ORDER STATUS: " << tempTrade.status << "\n";
            std::cout << "CURRENT BALANCE: " <<  tempTrade.currBalance << "\n";
            
        }
    }


    history[id] = tempTrade;
    
}

std::unordered_map<long int, Trade>& Broker::returnHistory(){
    return history;
}
std::unordered_map<long int, Order>& Broker::returnOrders(){
    return orders;
}