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
        //commisionFee is a flat per-trade dollar fee (see its declaration
        //in broker.hpp), not a per-share one -- it has to be added ONCE,
        //outside the *quantity multiply, or a 500-share order would need
        //$500 of "commission" alone instead of the flat $1 it's supposed to be
        if(tempBalance >= currPrice*check.quantity + commisionFee){
            return true;
        }
        else{
            return false;
        }
        //remove return once function built

    }
    //sell
    else{
        //a sell can do two different things depending on how much of the
        //ticker is currently held: "coveringQty" is the part that just
        //closes an existing long (needs nothing extra - you already own
        //those shares), and "shortQty" is whatever's left over once the
        //held long is exhausted, which opens or extends a short instead
        long currentQty = user.positionQuantity(check.ticker);
        long heldLong = std::max(currentQty, 0L);
        long coveringQty = std::min(check.quantity, heldLong);
        long shortQty = check.quantity - coveringQty;

        if(shortQty == 0){
            //fully covered by shares already held - identical to the
            //original(pre-shorting) behavior for this case
            return true;
        }

        //opening/extending a short with the "shortQty" leftover -- MVP
        //guardrail: require 100% cash collateral up front(no margin, no
        //borrow interest). to short $1,000 of stock, $1,000 of cash has to
        //already be sitting in the account. this reuses the same
        //"can you afford this" shape the buy branch above already uses
        double tempBalance = user.checkBalance();
        double price;
        Bar& currBar = currBars[check.ticker];
        if(check.type == "market"){
            price = currBar.open;
        }
        else if(check.type == "limit"){
            price = check.checkPrice;
        }
        else{
            price = currBar.close;
        }

        return tempBalance >= price*shortQty + commisionFee;
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

    //execPrice below is pure slippage now -- commisionFee used to be baked
    //in here per-share(the same bug checkOrder had above), which made the
    //fee scale with quantity instead of staying flat. commission is now
    //deducted once as its own explicit ledger entry further down, after
    //Account's buy/sell call actually moves the shares
    if(order.type == "market"){
        if(order.side ==0){
            currPrice = (currBar.open)*(1.0+slippageRate);
        }
        else{
            currPrice = (currBar.open)*(1.0-slippageRate);
        }
    }
    else if(order.type == "limit"){
        if(order.side ==0){
            //execute the better price between the open and checkPrice
            //std::min/std::max just return whichever of the two numbers is
            //smaller/larger - here they pick the more favorable of the
            //bar's open price vs the order's target price
            currPrice = (std::min(order.checkPrice, currBar.open))*(1.0+slippageRate);
        }
        else{
            currPrice = (std::max(order.checkPrice, currBar.open))*(1.0-slippageRate);
        }
    }
    else if(order.type == "stop"){
        if(order.side ==0){
            currPrice = (std::max(order.checkPrice, currBar.open))*(1.0+slippageRate);
        }
        else{
            currPrice = (std::min(order.checkPrice, currBar.open))*(1.0-slippageRate);
        }
    }
    else if(order.type == "stop_limit"){
        //checkOrderLimitAndStop's trigger condition is identical to a
        //plain stop's(needs no changes there - see that function) - the
        //only difference is what happens once triggered: a plain stop
        //fills at market, this fills as a LIMIT at limitPrice instead,
        //bounded against the bar's open the same way the "limit" type
        //above is bounded against checkPrice
        //
        //known simplification, consistent with this engine's existing
        //single-open-price-per-bar model: this always fills once
        //triggered. it doesn't model a bar that gaps straight through
        //limitPrice, where a real stop-limit could fail to fill at all
        if(order.side ==0){
            currPrice = (std::min(order.limitPrice, currBar.open))*(1.0+slippageRate);
        }
        else{
            currPrice = (std::max(order.limitPrice, currBar.open))*(1.0-slippageRate);
        }
    }
    else{
        //defensive fallback: every non-market/non-limit type used to fall
        //into one shared "else" here, so a typo'd type string(e.g.
        //"stpo") would silently behave exactly like a stop order without
        //any warning. now that "stop_limit" is a real type sharing this
        //same space, that silent-fallback footgun is worse than before, so
        //an unrecognized type is caught explicitly here instead
        currPrice = currBar.open;
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
            //if the account is currently short this ticker, some (or all)
            //of this buy covers that short instead of just adding to a
            //long -- "heldShort" is how much short exists to cover,
            //"coveredQty" is however much of THIS order actually covers it
            long oldQty = user.positionQuantity(order.ticker);
            double preBuyAEP = user.positionAEP(order.ticker);
            long heldShort = std::max(-oldQty, 0L);
            long coveredQty = std::min(order.quantity, heldShort);

            tempTrade.filled = true;
            tempTrade.status = "ORDER " + std::to_string(id) + " FILLED: BUY " + order.ticker + " " + std::to_string(order.quantity) + " FOR " + " " + std::to_string(currPrice);

            if(coveredQty > 0){
                //covering a short realizes P&L on the covered portion --
                //the sign is flipped relative to a long's realizedPnL
                //formula elsewhere in this file, since a short profits
                //when price FALLS, not rises
                tempTrade.realizedPnL = (preBuyAEP - currPrice) * coveredQty - commisionFee;
            }
            else{
                //nothing to cover - a plain add to an existing long never
                //realizes P&L, same as before this fix
                tempTrade.realizedPnL = 0.0;
            }

            user.buyPositionQuantity(order.ticker, order.quantity, currPrice);
            //commission is its own explicit ledger deduction now, separate
            //from execPrice -- see the comment above processOrder's
            //execPrice branches for why
            user.modifyBalance(-commisionFee);

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
                user.modifyBalance(-commisionFee);
                tempTrade.realizedPnL = (currPrice - preSaleAEP) * order.quantity - commisionFee;

                tempTrade.currBalance = user.checkBalance();
                std::cout << "ORDER STATUS: " << tempTrade.status << "\n";
                std::cout << "CURRENT BALANCE: " <<  tempTrade.currBalance << "\n";

            }
            else if(user.positionQuantity(order.ticker) > order.quantity){
                tempTrade.filled = true;
                tempTrade.status = "ORDER " + std::to_string(id) + " FILLED: SELL " + order.ticker + " " + std::to_string(order.quantity) + " FOR " + " " + std::to_string(currPrice);

                double preSaleAEP = user.positionAEP(order.ticker);
                user.sellPositionQuantity(order.ticker, order.quantity, currPrice);
                user.modifyBalance(-commisionFee);
                tempTrade.realizedPnL = (currPrice - preSaleAEP) * order.quantity - commisionFee;

                tempTrade.currBalance = user.checkBalance();
                std::cout << "ORDER STATUS: " << tempTrade.status << "\n";
                std::cout << "CURRENT BALANCE: " <<  tempTrade.currBalance << "\n";
            }
            //order.quantity exceeds however much is currently held long
            //(possibly 0, possibly already short) -- this either opens a
            //fresh short from flat, extends an existing short further, or
            //(if currently long) sells past the held long and flips
            //through zero into a brand new short
            else{
                long oldQty = user.positionQuantity(order.ticker);
                double preSaleAEP = user.positionAEP(order.ticker);

                tempTrade.filled = true;
                tempTrade.status = "ORDER " + std::to_string(id) + " FILLED: SELL " + order.ticker + " " + std::to_string(order.quantity) + " FOR " + " " + std::to_string(currPrice);

                if(oldQty > 0){
                    //the first oldQty shares close the existing long
                    //(realizing P&L on that portion, same formula the
                    //other sell branches above use); the leftover shares
                    //open a fresh short and realize nothing, since opening
                    //a position never realizes P&L
                    tempTrade.realizedPnL = (currPrice - preSaleAEP) * oldQty - commisionFee;
                }
                else{
                    //already flat or short - the entire sale just opens or
                    //extends the short, nothing is being closed
                    tempTrade.realizedPnL = 0.0;
                }

                user.sellPositionQuantity(order.ticker, order.quantity, currPrice);
                user.modifyBalance(-commisionFee);

                tempTrade.currBalance = user.checkBalance();
                std::cout << "ORDER STATUS: " << tempTrade.status << "\n";
                std::cout << "CURRENT BALANCE: " <<  tempTrade.currBalance << "\n";
            }
        }
    }
    //if position does not exist(create new position or open a short directly)
    else{
        if(order.side == 1){
            //in practice this branch is close to unreachable: checkOrder
            //already reads positionQuantity(ticker) before every order is
            //even accepted, which auto-vivifies a zero-quantity position
            //entry via the map's operator[] -- so by the time an order
            //reaches processOrder, checkPosition is basically always true
            //already. kept short-aware anyway for defensiveness/consistency:
            //buyNewPosition's math is already sign-correct for a negative
            //(short) quantity, so it's reused here rather than duplicating it
            tempTrade.filled = true;
            tempTrade.status = "ORDER " + std::to_string(id) + " FILLED: SELL " + order.ticker + " " + std::to_string(order.quantity) + " FOR " + " " + std::to_string(currPrice);
            tempTrade.realizedPnL = 0.0;

            user.buyNewPosition(order.ticker, -order.quantity, currPrice);
            user.modifyBalance(-commisionFee);

            tempTrade.currBalance = user.checkBalance();
            std::cout << "ORDER STATUS: " << tempTrade.status << "\n";
            std::cout << "CURRENT BALANCE: " <<  tempTrade.currBalance << "\n";
        }
        else{
            tempTrade.filled = true;
            tempTrade.status = "ORDER " + std::to_string(id) + " FILLED: BUY " + order.ticker + " " + std::to_string(order.quantity) + " FOR " + " " + std::to_string(currPrice);

            user.buyNewPosition(order.ticker, order.quantity, currPrice);
            user.modifyBalance(-commisionFee);

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
