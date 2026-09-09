#include "../include/strategy.hpp"
#include <cmath>

Strategy::Strategy(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::string symbol) : broker(b), user(u), connectBars(cBs), tradeHistory(history), ticker(symbol){
}

void Strategy::loadBar(){
    //this implementation works best for one stock
    Bar temp = connectBars.begin()->second;
    barHistory.push_back(temp);
}

//static_cast<long> converts the std::floor result(a double, since floor
//only works on floating point) into a whole-number long - std::floor
//rounds down to the nearest whole number(e.g 14.87 becomes 14.0), so this
//always sizes an order to whole shares, never a fraction of one
long Strategy::sizeBuyOrder(double price) const{
    return static_cast<long>(std::floor((user.checkBalance() * positionSizePct) / price));
}

long Strategy::sizeSellOrder(long currentQuantity) const{
    return static_cast<long>(std::floor(currentQuantity * positionSizePct));
}

void Strategy::placeBracketOrders(double entryPrice, long quantity, double stopLossPct, double takeProfitPct){
    //"Order{a, b, c, d, e}" is aggregate initialization - since Order is a
    //plain struct with no constructor of its own, you can build one by
    //listing its fields in order inside braces, instead of writing
    //stopOrder.ticker = ...; stopOrder.type = ...; on separate lines
    Order stopOrder{ticker, "stop", 1, quantity, entryPrice * (1.0 - stopLossPct)};
    Order limitOrder{ticker, "limit", 1, quantity, entryPrice * (1.0 + takeProfitPct)};
    broker.createOrder(stopOrder);
    broker.createOrder(limitOrder);
}
