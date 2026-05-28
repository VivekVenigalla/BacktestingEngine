#include <iostream>
#include <string>
#include "../include/csvParser.hpp"
#include "../include/account.hpp"


int main(){
    Parser test;
    std::map<std::string, Bar> newData = test.parse();
    std::cout<<newData["2015-01-13 00:00:00-05:00"].open<<" test\n";
    Account newAccount(1200.0);
    std::cout<<newAccount.checkBalance()<<"\n";
    double currPrice = newData["2015-01-13 00:00:00-05:00"].open;
    newAccount.buyNewPosition("AAPL", 20, currPrice);
    std::cout<<newAccount.positionQuantity("AAPL")<<"\n";
    std::cout<<newAccount.checkBalance()<<"\n";
}