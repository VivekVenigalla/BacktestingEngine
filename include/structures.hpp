#pragma once
#include <string>
#include <iostream>

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

    void print() const{
        std::cout<<ticker << " @ " << date << " :" << std::endl;
        std::cout<<"Open = " << open << std::endl;
        std::cout<<"High = " << high << std::endl;
        std::cout<<"Low = " << low << std::endl;
        std::cout<<"Close = " << close << std::endl;
        std::cout<<"Volume = " << volume << std::endl;
    }
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
    //assuming there is a limit or stop order
    double checkPrice;
};

//current value of a pertaining ticker
//can iterate into a std::vector for a portfolio
struct Position{
    std::string ticker;
    long quantity;
    double average_entry_price;
//add more if needed
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
    bool side;
    long quantity;
    double checkPrice;
    //figure out the commmision calculation
    double commision;
    std::string status;
    double currBalance;
};
