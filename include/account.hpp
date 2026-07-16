#pragma once
#include "structures.hpp"
#include <unordered_map>
#include <vector>
//requires structures.hpp for the position struct

//

class Account{
    public:

        //types of instantiation
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
        
        std::unordered_map<std::string, Position> returnPositions();
        std::string id;

    private:
        double balance = 10000.0;
        //map of all the positions in the account
        //positions.first => ticker, positions.second=> Position struct
        std::unordered_map<std::string, Position> positions;
        //map of all positions at every time instant
        int numPositions;

        
};