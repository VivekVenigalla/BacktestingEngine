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
#include "../include/performanceEval.hpp"
#include "../include/logger.hpp"

int main(){

    //implement stock selection here later when python csv downloader is integrated
    std::vector<std::string> tickers = {"AAPL"};
    double initBalance = 1200.0;
    
    
    Data feed("AAPL");
    Bar& bar = feed.getBar();
    Account newAccount(initBalance);
    Broker newBroker(newAccount, bar);
    Logger logger;
    //history of orders and trades
    std::unordered_map<long int, Trade>& historyRef = newBroker.returnHistory();
    std::unordered_map<long int, Order>& orderRef = newBroker.returnOrders();
    std::unordered_map<std::string, Bar> tempBarLog;
    std::unordered_map<std::string, Position> tempPositionLog;

    tempBarLog["AAPL"] = bar;

    //history of positions
    Metrics calculate(newAccount, historyRef, tickers);
    
    
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
    
    
    while(feed.hasMoreData()){
        //if the feed has more data, then first update the strategy with the data
        bar = feed.getBar();
        tempBarLog["AAPL"] = bar;
        
        strategy->loadBar();
        strategy->runBar();
        newBroker.checkLoop();
        std::cout<<"Current Bar: " << bar.date << " Current Balance: " << newAccount.checkBalance() << "\n";
        feed.nextBar();

        //load snapshot
        logger.logSnapshot(bar.date, tempBarLog, newAccount.checkBalance(), newAccount.accountValue(tickers, bar.close), newAccount.returnPositions());
        
    }
    //full history of trades that were executed
    //iterate over the historyRef
    for(const auto& [key, value] : historyRef){
        //using the print function of the trade struct
        if(value.filled){
            value.print();
        }
        
    }
    
    //metrics
    double returns = calculate.totalReturn(initBalance, bar.close);
    double cagr = calculate.cagr(initBalance, bar.close, 10);
    
    std::cout<<"Total return: " << returns << std::endl;
    std::cout<<"CAGR: " << cagr << std::endl;

    //checking logger
    //date : 2015-01-12 00:00:00-05:00
    logger.printSnapshot("2015-04-20 00:00:00-05:00");
    
    
}