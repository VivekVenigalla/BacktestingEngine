#include <iostream>
#include <string>
#include <memory>
#include "../include/csvParser.hpp"
#include "../include/account.hpp"
#include "../include/broker.hpp"
#include "../include/dataFeed.hpp"
#include "../include/strategy.hpp"
#include "../include/createStrat.hpp"
//check if this below is needed
#include "../include/strategies/smaCross.hpp"

int main(){

    //implement stock selection here later when python csv downloader is integrated
    std::vector<std::string> tickers = {"AAPL"}
    Data feed("AAPL");
    Bar& bar = feed.getBar();
    Account newAccount(1200.0);
    Broker newBroker(newAccount, bar);
    std::unordered_map<long int, Trade>& historyRef = newBroker.returnHistory();
    std::unordered_map<long int, Order>& orderRef = newBroker.returnOrders();
    std::string stratType;
    
    std::cout << "Strategy input : ";
    std::cin >> stratType;
    std::cout << std::endl;

    //strategies at this point can only use one stock in reference, integrate multiple stocks option later
    std::unique_ptr<Strategy> strategy = StrategyFactory::create(newBroker, newAccount, bar, historyRef, "AAPL", stratType);

    strategy->init();
    
    std::cout<<"Current Bar: " << bar.date << " Current Balance: " << newAccount.checkBalance() << "\n";
    feed.nextBar();
    std::cout<<"Current Bar: " << bar.date << " Current Balance: " << newAccount.checkBalance() << "\n";
    
    //main loop
    //check if the feed has more data
    int currBar = 1;
    while(feed.hasMoreData()){
        //if the feed has more data, then first update the strategy with the data
        bar = feed.getBar();
        strategy->loadBar();
        strategy->runBar();
        newBroker.checkLoop();
        std::cout<<"Current Bar: " << bar.date << " Current Balance: " << newAccount.checkBalance() << "\n";
        feed.nextBar();
    }
    
    

    
    /*
    newAccount.buyNewPosition("PLACE", 20, bar.open);
    //std::cout<<newAccount.positionQuantity("AAPL")<<"\n";
    std::cout<<newAccount.checkBalance()<<"\n";
    bar.print();
    


    

    Order newOrder = {"PLACE", "market", 0, 10, -1.0};
    newBroker.createOrder(newOrder);
    historyRef[1].print();
    
    //currBar = newData["2015-01-29 00:00:00-05:00"];
    if(feed.hasMoreData()){
        feed.nextBar();
        bar = feed.getBar();
        std::cout<<bar.high<<"\n";
    }
    bar.print();
    newOrder = {"PLACE", "limit", 1, 15, 26.0};
    int id = newBroker.createOrder(newOrder);
    std::cout << id<< " " << newAccount.positionQuantity("PLACE") <<  std::endl;
    newBroker.checkLoop();
    
    //historyRef[2].print();
    */
}