#include "../include/broker.hpp"
#include <iostream>
#include <algorithm>

//WORK IN PROGRESS
//FIX create order and process order, while building method and means for obtaining the data values.

//"user(account),currBars(connectBar)" here is a member initializer list -
//it runs before the constructor body{} starts, and is required for
//reference members like Account& user, since a reference has to be bound
//to something the moment it's created(unlike a normal variable, you can't
//assign a reference later)
Broker::Broker(Account& account, std::unordered_map<std::string,Bar>& connectBar) : user(account),currBars(connectBar) {
    //no additional construction needed for now, since user and currBar is already referenced
}

Broker::Broker(Account& account, std::unordered_map<std::string,Bar>& connectBar, double commision, double slippage, std::string ID) :user(account),currBars(connectBar),commisionFee(commision),slippageRate(slippage),id(ID){

}



//returns the ID of the order if it is exectable at request time. If not then returns 0
//an order's life cycle: createOrder() checks it's valid, then either drops
//it in the "orders" map to wait(pending), or logs an immediate failure to
//"history". checkLoop() is what later moves a pending order out of "orders"
//and into "history" once it actually fills(see processOrder below)
int Broker::createOrder(Order newOrder){
    //the Broker must check if the order is a market, so that it will go into the market category
    //there is no need to have a map for market orders since they are immediately executed once requested
    //orders are checled immediately to see if they are viable with the current funds/shares
    if(checkOrder(newOrder)){
        orders[tempID] = newOrder;
        //this commented-out block is old code from before market orders
        //were changed to always wait for checkLoop() - keeping it here as
        //a note of the previous design, but it's dead code, never runs
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
        tempTrade.side = newOrder.side;
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
    tempTrade.side = newOrder.side;
    tempTrade.quantity = newOrder.quantity;
    tempTrade.checkPrice = newOrder.checkPrice;
    tempTrade.filled = false;
    tempTrade.status = "ORDER " + std::to_string(orderID) + " CANCELLED : " + reason;
    //create trade history entry
    history[orderID] = tempTrade;
    //erase the order
    orders.erase(orderID);
}

//param:reference to the order to check to preserve memory and efficiency of not having to copy the order again
//return: bool=> true if executable at time requested and false if not
//this only checks "can this order be afforded/fulfilled right now" - it
//does NOT check whether a limit/stop order's target price has been
//reached, that's checkOrderLimitAndStop's job further down this file
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

//this is the main "tick" the simulation calls once per bar - it walks every
//still-pending order and decides whether this bar's price makes it fillable
void Broker::checkLoop(){
    //.begin and .end provide the iterators for the loop so we can iterate through the unordered_map
    //an iterator is like a pointer that walks through a container one
    //element at a time - it != orders.end() means "not past the last
    //element yet", and it->first/it->second read the current entry's
    //key/value(same idea as the [key,value] structured bindings used
    //elsewhere, just the older, more manual way of writing it)
    for(auto it = orders.begin();it!=orders.end();){
        //since it is a iterator, in order to obtain the id, we use it->first

        //check if it is market and immediately process if so
        if(it->second.type == "market"){
            processOrder(it->first, it->second);
            //orders.erase(it) removes the current entry and returns an
            //iterator to the NEXT valid entry - you can't just do ++it
            //after erasing, because the iterator you were holding is now
            //invalid(it pointed at memory that just got freed). that's why
            //this loop uses "it = orders.erase(it)" instead of a plain ++it
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
            //passed required price but lack of funds or shares so order is still held
            else{
                //even if there are no funds or shares available, dont cancel the order unless the strategy says so
                ++it;
                //old, unused alternate ending for the branch above - would
                //have cancelled the order here instead of leaving it
                //pending. never runs, kept only as a historical note
                /*
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
                continue;*/
            }
        }
        else{
            //only increments if there was no deletion so we don't skip an order
            ++it;
        }
    }
}


//checks whether a limit/stop order's TARGET price has been reached by this
//bar - separate from checkOrder above, which only checks affordability.
//a limit order wants a better-or-equal price than its target; a stop order
//wants the market to have moved PAST its target(the opposite direction) -
//that's why the side==0/side==1 branches are flipped between the two halves
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

//actually fills an order: works out the real execution price(including
//slippage and commission), then tells Account to move the cash/shares, and
//logs the outcome as a Trade. by the time this runs, checkOrder and
//checkOrderLimitAndStop have already confirmed the order is fillable
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
    }
    else if(order.type == "limit"){
        if(order.side ==0){
            //execute the better price between the open and checkPrice
            //std::min/std::max just return whichever of the two numbers is
            //smaller/larger - here they pick the more favorable of the
            //bar's open price vs the order's target price
            currPrice = (std::min(order.checkPrice, currBar.open))*(1.0+slippageRate)+commisionFee;
        }
        else{
            currPrice = (std::max(order.checkPrice, currBar.open))*(1.0-slippageRate)-commisionFee;
        }
    }
    else{
        if(order.side ==0){
            currPrice = (std::max(order.checkPrice, currBar.open))*(1.0+slippageRate)+commisionFee;
        }
        else{
            currPrice = (std::min(order.checkPrice, currBar.open))*(1.0-slippageRate)-commisionFee;
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

                //realizedPnL needs the AEP from BEFORE the sell, so it has
                //to be read here, before sellAllPosition() updates the
                //position below
                double preSaleAEP = user.positionAEP(order.ticker);
                user.sellAllPosition(order.ticker, currPrice);
                tempTrade.realizedPnL = (currPrice - preSaleAEP) * order.quantity;

                tempTrade.currBalance = user.checkBalance();
                std::cout << "ORDER STATUS: " << tempTrade.status << "\n";
                std::cout << "CURRENT BALANCE: " <<  tempTrade.currBalance << "\n";

            }
            else if(user.positionQuantity(order.ticker) > order.quantity){
                tempTrade.filled = true;
                tempTrade.status = "ORDER " + std::to_string(id) + " FILLED: SELL " + order.ticker + " " + std::to_string(order.quantity) + " FOR " + " " + std::to_string(currPrice);

                double preSaleAEP = user.positionAEP(order.ticker);
                user.sellPositionQuantity(order.ticker, order.quantity, currPrice);
                tempTrade.realizedPnL = (currPrice - preSaleAEP) * order.quantity;

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

void Broker::reset(){
    orders.clear();
    history.clear();
}

std::unordered_map<long int, Trade>& Broker::returnHistory(){
    return history;
}
std::unordered_map<long int, Order>& Broker::returnOrders(){
    return orders;
}
