#include <iostream>
#include <string>
#include "../include/csvParser.hpp"
#include "../include/account.hpp"
#include "../include/broker.hpp"
#include "../include/dataFeed.hpp"

int main(){
    Data feed;
    Bar& bar = feed.getBar();
    Account newAccount(1200.0);
    Broker newBroker(newAccount, bar);
    std::unordered_map<long int, Trade>& historyRef = newBroker.returnHistory();
    std::unordered_map<long int, Order>& orderRef = newBroker.returnOrders();
    
    newAccount.buyNewPosition("PLACE", 20, bar.open);
    //std::cout<<newAccount.positionQuantity("AAPL")<<"\n";
    std::cout<<newAccount.checkBalance()<<"\n";

    


    

    Order newOrder = {"PLACE", "market", 0, 10, -1.0};
    newBroker.createOrder(newOrder);
    historyRef[1].print();
    
    //currBar = newData["2015-01-29 00:00:00-05:00"];
    if(feed.hasMoreData()){
        feed.nextBar();
        std::cout<<bar.high<<"\n";
    }
    newOrder = {"PLACE", "limit", 1, 15, 26.0};
    int id = newBroker.createOrder(newOrder);
    std::cout << id<< " " << newAccount.positionQuantity("PLACE") <<  std::endl;
    newBroker.checkLoop();
    //historyRef[2].print();
}