#include <iostream>
#include <string>
#include "../include/csvParser.hpp"
#include "../include/account.hpp"
#include "../include/broker.hpp"

int main(){
    Parser test;
    //we do not create a reference to the data since in the csvParser, the data is alreday temporary since it is a local var
    std::map<std::string, Bar> newData = test.parse();
    //std::cout<<newData["2015-01-13 00:00:00-05:00"].open<<" test\n";
    Account newAccount(1200.0);
    std::cout<<newAccount.checkBalance()<<"\n";
    double currPrice = newData["2015-01-13 00:00:00-05:00"].open;
    newAccount.buyNewPosition("AAPL", 20, currPrice);
    std::cout<<newAccount.positionQuantity("AAPL")<<"\n";
    std::cout<<newAccount.checkBalance()<<"\n";

    Bar currBar = newData["2015-01-13 00:00:00-05:00"];
    
    //create broker class
    Broker newBroker(newAccount, currBar);

    std::unordered_map<long int, Trade>& historyRef = newBroker.returnHistory();
    std::unordered_map<long int, Order>& orderRef = newBroker.returnOrders();
    Order newOrder = {"PLACE", "market", 0, 10, 0.0};
    newBroker.createOrder(newOrder);

    
    currBar = newData["2015-01-29 00:00:00-05:00"];
    newOrder = {"PLACE", "limit", 1, 15, 26.0};
    newBroker.createOrder(newOrder);
    newBroker.checkLoop();
    
}