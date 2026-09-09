#include "../include/account.hpp"
#include <iostream>

//types of instantiation
//1. only starting balance included
//2. ask for initial positions
Account::Account(double initBalance){
    balance = initBalance;
}

//heads up: this constructor is unfinished/dead code, left over from an
//early idea and never completed - it asks for a number of positions, then
//immediately throws an int(12) purely so the catch block right below can
//catch it and print a message, and the for loop after that never actually
//builds any positions("will implement at a later point"). throwing just to
//immediately catch your own throw is not a normal c++ pattern anywhere else
//in this codebase - it's exactly what it looks like, a placeholder that was
//never finished. nothing currently calls this constructor
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

//": id(ID)" is a member initializer list - it runs before the constructor's
//{} body starts, and is the normal c++ way to set a member field from a
//constructor argument. here it copies the ID parameter into the id field
Account::Account(double initBalance, std::vector<std::string> tickers, std::string ID) : id(ID){
    balance = initBalance;
    initial = initBalance;
    for(int i = 0; i < tickers.size(); i++){
        buyNewPosition(tickers[i], 0, 0.0);
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

void Account::setBalance(double newbalance){
    balance = newbalance;
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

//buying more of a position you already hold has to blend the new shares'
//price into the existing average entry price(aep), weighted by how many
//shares came from each purchase - not just a plain average of the two
//prices(that would wrongly treat a 5-share buy and a 500-share buy as
//equally important to the average). the formula is:
//newAEP = (oldQty*oldAEP + newQty*newPrice) / (oldQty + newQty)
//oldQuantity/oldAEP have to be read BEFORE quantity gets updated below,
//since the formula needs the position's state as it was right before this
//buy, not after
void Account::buyPositionQuantity(std::string ticker, long quantityChange, double entryPrice){
    long oldQuantity = positions[ticker].quantity;
    double oldAEP = positions[ticker].average_entry_price;
    positions[ticker].quantity += quantityChange;
    positions[ticker].average_entry_price = (oldQuantity*oldAEP + quantityChange*entryPrice) / (oldQuantity + quantityChange);
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

//unordered_map has no "contains this key?" method by itself in older c++
//standards, so .count(ticker) is the standard workaround - it returns how
//many entries have that key(always 0 or 1 here, since tickers are unique),
//so > 0 means "yes, this ticker exists in the map"
bool Account::checkPosition(std::string ticker){
    return positions.count(ticker) > 0;
}

void Account::reset(){
    //"auto& [key, value]" is a structured binding(c++17) - positions is a
    //map<string, Position>, and this unpacks each entry's key/value pair
    //into two named variables directly, instead of writing it->first and
    //it->second. the & means value is a reference to the real Position
    //inside the map, so modifying value.quantity below actually changes it
    for(auto& [key, value] : positions){
        value.quantity = 0;
    }
    balance = initial;
}

double Account::accountValue(std::unordered_map<std::string, double> currPrices){
    double temp = balance;
    for(const auto& [key, value] : currPrices){
        if(checkPosition(key)){
            temp+=positionValue(key, value);
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
