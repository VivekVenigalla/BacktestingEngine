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
    //std::cout<<newAccount.checkBalance()<<"\n";
    double currPrice = newData["2015-01-13 00:00:00-05:00"].open;
    newAccount.buyNewPosition("PLACE", 20, currPrice);
    //std::cout<<newAccount.positionQuantity("AAPL")<<"\n";
    std::cout<<newAccount.checkBalance()<<"\n";

    Bar currBar = newData["2015-01-13 00:00:00-05:00"];
    //currBar.print();
    //create broker class
    Broker newBroker(newAccount, currBar);

    std::unordered_map<long int, Trade>& historyRef = newBroker.returnHistory();
    std::unordered_map<long int, Order>& orderRef = newBroker.returnOrders();
    Order newOrder = {"PLACE", "market", 0, 10, -1.0};
    newBroker.createOrder(newOrder);
    historyRef[1].print();
    
    currBar = newData["2015-01-29 00:00:00-05:00"];
    newOrder = {"PLACE", "limit", 1, 15, 26.0};
    int id = newBroker.createOrder(newOrder);
    std::cout << id<< " " << newAccount.positionQuantity("PLACE") <<  std::endl;
    newBroker.checkLoop();
    historyRef[2].print();
}