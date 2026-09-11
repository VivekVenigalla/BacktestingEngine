//this file defines the plain data types the whole engine passes around.
//nothing in here does real work(no strategy logic, no order matching) - it
//is just the "nouns" of the program(a Bar, an Order, a Trade...) that every
//other file includes and builds on top of

#pragma once
//pragma once is a header guard. if two different files both #include this
//header, the compiler would normally paste its contents in twice and fail
//to compile("Bar already defined"). pragma once tells the compiler "only
//ever include this file once per compiled file, no matter how many times
//it gets #included", so every other header can safely include structures.hpp
//without worrying about duplicate definitions
#include <string>
#include <iostream>
#include <unordered_map>
#include <vector>
//NOTE: includes in general basically mean that the code in 
//that correpsonding .hpp file is directly pasted into the code

//data considerations(keep in csv parser)
//consider the width of time intervals for ohcl data

//struct is basically the same thing as class in c++, except everything is
//public by default(class defaults to private). these are used here since
//Bar/Order/Position/Trade are just plain bundles of data with no need to
//hide their fields from the rest of the program
//these are useful for storing custom objects of data and tend to not have methods
//that does not mean they cannot have methods
//each of these structs has a print method for debugging purposes

//one candle/bar of price data for a single ticker on a single date
//ohlc = open, high, low, close - the four prices that summarize how a stock
//traded over one time period(here, one day)
struct Bar{
    std::string ticker;
    std::string date;
    double open;
    double close;
    double high;
    double low;
    long volume;

    //the "const" after a method means this function promises not to modify
    //any of the struct's fields. it only reads data and prints it, so
    void print() const;
};

//a request to the broker to buy or sell shares. the strategy fills one of
//these out and hands it to Broker::createOrder() - the strategy never
//touches the account balance or positions directly, only the broker does
//this prevents over reach bias and also allows simulation of slippage and 
//logic for filling stop and limit orders in a seperate file
struct Order{
    std::string ticker;
    //type is a plain string instead of a proper enum(a fixed list of named
    //values) for simplicity, but it only ever holds one of:
    //market - execute immediately at the current price, no target price
    //limit - only buy at or below(or sell at or above) a target price
    //stop - only trigger once price crosses a target, 
    //then trade at market(best possible price)
    std::string type; 
    int side; //side is an int standing in for buy(0) or sell(1) instead of a proper
    long quantity; //quantity as a integer
    //(this market does not allow partial orders)
    //the target price for limit/stop orders. market orders don't use one,
    //so checkPrice is set to -1 as a "not applicable" placeholder
    double checkPrice;
    //only meaningful for type=="stop_limit": checkPrice is still the stop
    //TRIGGER price(same as a plain stop order), but once triggered, the
    //order fills as a limit at limitPrice instead of at market. defaulted
    //to -1("not applicable") so every existing Order{...} aggregate
    //initialization elsewhere in the codebase keeps compiling unchanged -
    //this new field just falls back to its default when a caller doesn't
    //list it
    double limitPrice = -1;

    void print() const;
};

//how many shares of one ticker the account currently holds, and at what
//average price they were bought. one Account can hold many Positions(one
//per ticker), tracked in Account's own unordered_map<string, Position>
struct Position{
    std::string ticker;
    long quantity;
    double average_entry_price; //average price for each buy order

    void print() const;
//add more if needed
};

//one full bar-by-bar recording of a simulation, used to write the equity
//curve csv and to answer "what did the account look like on this date".
//each field below is a std::vector, and index i across every vector
//refers to the same point in time - so dates[5], balances[5], bars[5] etc
//all describe the same bar. this "parallel arrays" layout is a memory/speed
//tradeoff: it avoids one big struct-per-snapshot allocation, at the cost of
//needing to keep every vector's length in sync by hand(Logger::logSnapshot
//is the only place that's allowed to push_back onto these)
struct History{
    //handles multiple tickers - each element of "bars" and "positions" is a
    //map from ticker name to that ticker's Bar/Position on that date, so a
    //multi-asset simulation can log more than one instrument per snapshot

    //since the logger synchronizes all the values at a particular instant, 
    //we can use vectors and rely on their index values given from the date vector
    std::vector<std::string> dates;
    std::vector<std::unordered_map<std::string, Bar>> bars; //bars for each data feed
    std::vector<double> balances;
    std::vector<double> totalEquity;
    std::vector<double> drawDown;
    std::vector<std::unordered_map<std::string, Position>> positions;
    //uses the lookup map in the logger
    void print_with_date(int index) const;
};

//a record of what actually happened when the broker tried to fill an Order.
//an Order is a request; a Trade is the outcome(filled or not, at what
//price, for how much profit/loss). every Order that ever reaches
//Broker::createOrder() ends up as exactly one Trade in the broker's
//history map, keyed by the same id the Order was given
struct Trade{
    std::string ticker;
    //the price the trade actually executed at, including slippage and
    //commission - this can differ from Order::checkPrice, which was only
    //ever the target/trigger price, not the real fill price
    double execPrice;
    std::string type;
    int side;
    long quantity;
    double checkPrice;
    //mirrors Order::limitPrice above, for stop_limit trades only - see
    //that field's comment for what it means
    double limitPrice = -1;
    //figure out the commmision calculation
    double commision;
    //true if the broker actually filled this trade, false if it was
    //rejected(not enough cash/shares) or is still waiting to trigger
    bool filled;
    //a human-readable explanation of what happened to this trade, useful
    //for debugging - this is not a proper error code, just a logged string
    std::string status;
    double currBalance;
    //realized profit/loss for a filled sell, measured against the average
    //entry price the position had right before this sell. 0.0 for buys and
    //for trades that never filled, where a profit/loss doesn't apply
    double realizedPnL = 0.0;
    //the bar date this trade happened on(copied straight from Bar::date,
    //hence the same std::string type), so a trade can be placed back onto
    //an equity curve's x-axis later. defaults to empty since it's set right
    //alongside every other field wherever a Trade gets built
    std::string date = "";

    void print() const;
};
