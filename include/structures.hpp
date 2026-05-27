#include <string>

//data considerations(keep in csv parser)
//consider the width of time intervals for ohcl data

//struct for a ohcl data plot
//contains the date, open and close price, highest and lowest price in the period of measurement, and volume
struct Bar{
    string date;
    double open;
    double close;
    double high;
    double low;
    long volume;
};

//struct for a order
//contains the information on the ticker and the type of order
struct Order{
    string ticker;
    string type;
    //buy(0) sell(1)
    bool side;
    long quantity;
    //assuming there is a limit order
    double limit_price;
};

//current value of a pertaining ticker
//can iterate into a std::vector for a portfolio
struct Position{
    string ticker;
    long quantity;
    double average_entry_price;
//add more if needed
};

//an order will be for contacting the broker class to transfer stocks
//this struct will be to keep a history off all trades in history
//assume that each trade will pertain to an order by its index, as they are all fulfilled in succession
//consider time slippage
struct Trade{
    double exec_price;
    //figure out the commmision calculation
    double commision;
};
