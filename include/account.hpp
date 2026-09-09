#pragma once
#include "structures.hpp"
#include <unordered_map>
#include <vector>
//requires structures.hpp for the position struct

//Account is the ledger - it owns the cash balance and every open position,
//and knows nothing about orders, brokers, or strategies. Broker is the only
//class that's supposed to call these methods("the broker moves money, the
//account just remembers how much there is")

class Account{
    public:

        //c++ lets you declare the same function name multiple times as long
        //as the parameter list is different - this is called overloading.
        //the compiler picks which one to call based on what arguments you
        //pass in. these three constructors give three ways to build an
        //Account:
        //1. only starting balance included
        //2. ask for initial positions
        //3. provide initial positions
        Account(double initBalance);

        Account(double initBalance, bool initPos);

        Account(double initBalance, std::vector<std::string> tickers, std::string ID);

        //helper functions
        double checkBalance();
        void modifyBalance(double modifier);
        void setBalance(double newbalance);
        void buyNewPosition(std::string ticker, long quantity, double entryPrice);
        void buyPositionQuantity(std::string ticker, long quantityChange, double entryPrice);
        void sellPositionQuantity(std::string ticker, long quantityChange, double entryPrice);
        void sellAllPosition(std::string ticker, double currentPrice);

        double positionAEP(std::string ticker); //AEP => Average Entry Price
        long positionQuantity(std::string ticker);
        double positionValue(std::string ticker, double currPrice);
        bool checkPosition(std::string ticker);
        //need to create a unordered map of all prices for all tickers
        double accountValue(std::unordered_map<std::string, double> currPrices);
        //double checkTotalEquity();
        void reset();

        std::unordered_map<std::string, Position> returnPositions();
        std::string id;

    private:
        double initial = 0.0;
        double balance = 10000.0;
        //an unordered_map is a hash table - it looks up a Position by its
        //ticker string in constant time(o(1)) instead of scanning a list,
        //which matters since positions get looked up on almost every bar
        //map of all the positions in the account
        //positions.first => ticker, positions.second=> Position struct
        std::unordered_map<std::string, Position> positions;
        //map of all positions at every time instant
        int numPositions;


};
