#include "../include/account.hpp"
#include <iostream>

//types of instantiation
//1. only starting balance included
//2. ask for initial positions
Account::Account(double initBalance){
    balance = initBalance;
}

Account::Account(double initBalance, bool initPos){
    balance = initBalance;
    //ask the user for all of the positions
    std::cout << "Input the number of positions here: ";
    try{
        std::cin>>numPositions;
        throw 12;
    }
    catch (int e){
        std::cout << "Exception raised: " << e << "\n";
    }
    for(int i = 0; i < numPositions; i++){
        //will implement at a later point
        std::cout << "Here";
    }
}

//helper functions

//balance
double Account::checkBalance(){
    return balance;
}

void Account::modifyBalance(double modifier){
    balance += modifier;
}

//creating positions, deleting, and modifiying
//only assuming you can only buy and sell quantity of a position and nothing else
void Account::buyNewPosition(std::string ticker, long quantity, double entryPrice){
    Position newPos;
    newPos.ticker = ticker;
    newPos.quantity = quantity;
    newPos.average_entry_price = entryPrice;
    positions.insert({newPos.ticker, newPos});
    balance -= quantity*entryPrice;
}

void Account::buyPositionQuantity(std::string ticker, long quantityChange, double entryPrice){
    positions[ticker].quantity += quantityChange;
    positions[ticker].average_entry_price = (positions[ticker].average_entry_price + entryPrice)/2.0;
    balance -= quantityChange*entryPrice;
}

void Account::sellPositionQuantity(std::string ticker, long quantityChange, double currentPrice){
    positions[ticker].quantity -= quantityChange;
    balance += quantityChange*currentPrice;
}

void Account::sellAllPosition(std::string ticker, double currentPrice){
    balance+=positions[ticker].quantity*currentPrice;
    //since the graph should be able to indicate also when the strategy sells all of a position, the position is not erased
    positions[ticker].quantity = 0;
    
}

//average entry price and quantity of a position and presence of position
double Account::positionAEP(std::string ticker){
    return positions[ticker].average_entry_price;
}

long Account::positionQuantity(std::string ticker){
    return positions[ticker].quantity;
}

double Account::positionValue(std::string ticker, double currPrice){
    return positions[ticker].quantity*currPrice;
}

bool Account::checkPosition(std::string ticker){
    return positions.count(ticker) > 0;
} 

double Account::accountValue(const std::vector<std::string> allTickers, double currPrice){
    double temp = balance;
    for(const auto& element : allTickers){
        if(checkPosition(element)){
            //std::cout<<"Position Quantity: "<<positionQuantity(element)<<std::endl;
            //std::cout<<"AEP: "<<positionAEP(element)<<std::endl;
            temp+=positionValue(element, currPrice);
        }
    }
    return temp;
}

std::unordered_map<std::string, Position> Account::returnPositions(){
    return positions;
}

//this function will be implemented at a later point
//total equity = cash value + value of all positions
//instead of passing all args at once, create a file that manages data for all of the tickers history
/*
double Account::checkTotalEquity(double currentPrice){
    
}
*/

