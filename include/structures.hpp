#pragma once
#include <string>
#include <iostream>
#include <unordered_map>
#include <vector>
//data considerations(keep in csv parser)
//consider the width of time intervals for ohcl data

//struct for a ohcl data plot
//contains the date, open and close price, highest and lowest price in the period of measurement, and volume
struct Bar{
    std::string ticker;
    std::string date;
    double open;
    double close;
    double high;
    double low;
    long volume;

    void print() const;
};

//struct for a order
//contains the information on the ticker and the type of order
struct Order{
    std::string ticker;
    //types of orders:
    //1. market
    //2. limit
    //3. stop
    std::string type;
    //buy(0) sell(1)
    int side;
    long quantity;
    //assuming there is a limit or stop order else == -1
    double checkPrice;

    void print() const;
};

//current value of a pertaining ticker
//can iterate into a std::vector for a portfolio
struct Position{
    std::string ticker;
    long quantity;
    double average_entry_price;

    void print() const;
//add more if needed
};

//equity snapshot that allows plotting
struct History{
    /*std::string ticker;
    std::string date;
    Bar& bar;
    double balance;
    double totalValue;
    //position(second) snap shot for each ticker(first element)
    std::unordered_map<std::string, Position>;*/

    //handles multiple tickers
    std::vector<std::string> dates;
    std::vector<std::unordered_map<std::string, Bar>> bars;
    std::vector<double> balances;
    std::vector<double> totalEquity;
    std::vector<double> drawDown;
    std::vector<std::unordered_map<std::string, Position>> positions;
    //uses the lookup map in the logger
    void print_with_date(int index) const;
};

//an order will be for contacting the broker class to transfer stocks
//this struct will be to keep a history off all trades in history
//assume that each trade will pertain to an order by its index, as they are all fulfilled in succession
//consider time slippage

//the id of a trade is the same as a order
//trades are essentially the same as a order but they include the exec_price to account for slippage and also the commision as well
//these trades are logged on the backtester user's choice to evaluate the strength of the strategy
struct Trade{
    std::string ticker;
    double execPrice;
    std::string type;
    int side;
    long quantity;
    double checkPrice;
    //figure out the commmision calculation
    double commision;
    bool filled;
    std::string status;
    double currBalance;

    void print() const;
};
