#include <iostream>
#include <string>
#include <fstream>
#include <memory>
#include "../include/csvParser.hpp"
#include "../include/account.hpp"
#include "../include/broker.hpp"
#include "../include/dataFeed.hpp"
#include "../include/strategy.hpp"
#include "../include/createStrat.hpp"
//check if this below is needed
#include "../include/strategies/smaCross.hpp"
#include "../include/strategies/bollBand.hpp"
#include "../include/strategies/donChannel.hpp"
#include "../include/performanceEval.hpp"
#include "../include/logger.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

int main(){
    //parse the json config file and obtain the settings for sim
    std::string JSON_PATH = "../config/simConfig.json";
    std::ifstream file(JSON_PATH);
    json config;
    file >> config;
    //implement stock selection here later when python csv downloader is integrated

    std::vector<std::string> tickers = {"AAPL"};
    double initBalance = config["account"]["initial_balance"].get<double>();
    

    std::vector<Data> feeds;
    //the reserve allows me to allow enough space for the tickers
    feeds.reserve(tickers.size()); 

    std::unordered_map<std::string, Bar> bars;
    
    //create data feeds with csvParser
    for(const auto& tempTicker : tickers){
        //automatically creates an object
        feeds.emplace_back(tempTicker); 
    }

    //create bar objects for broker and strategy
    for(auto& feed : feeds){
        bars[feed.ticker] = feed.getBar();
    }

    Account newAccount(initBalance, tickers);
    //modify so it has commision and slippage as well
    Broker newBroker(newAccount, bars, 1.0, 0.0005);
    Logger logger;
    //history of orders and trades
    std::unordered_map<long int, Trade>& historyRef = newBroker.returnHistory();
    std::unordered_map<long int, Order>& orderRef = newBroker.returnOrders();
    std::unordered_map<std::string, Bar> tempBarLog;
    std::unordered_map<std::string, Position> tempPositionLog;


    //history of positions
    Metrics calculate(newAccount, historyRef, tickers);
    
    
    std::string stratType;
    
    std::cout << "Strategy input : ";
    std::cin >> stratType;
    std::cout << std::endl;

    std::unique_ptr<Strategy> strategy = StrategyFactory::create(newBroker, newAccount, bars, historyRef, "AAPL", stratType);

    strategy->init();
    /*
    std::cout<<"Current Bar: " << bar.date << " Current Balance: " << newAccount.checkBalance() << "\n";
    feed.nextBar();
    std::cout<<"Current Bar: " << bar.date << " Current Balance: " << newAccount.checkBalance() << "\n";
    */
    //main loop
    //check if the feed has more data
    std::unordered_map<std::string, double> currPrices;
    //iterate over one feed(this simulation assumes that the feeds are identical in size)
    while(feeds[0].hasMoreData()){
        //if the feed has more data, then first update the strategy with the data
        for(auto& tempFeed : feeds){
            bars[tempFeed.ticker] = tempFeed.getBar();
            tempBarLog[tempFeed.ticker] = bars[tempFeed.ticker];
            //the account alue will use the close value of the bar
            currPrices[tempFeed.ticker] = bars[tempFeed.ticker].close;
        }
        //check this one
        newBroker.checkLoop();
        //loadBar is overwritten if the strategy required multiple tickers
        strategy->loadBar();
        strategy->runBar();
        std::cout<<"Current Date: " << bars.begin()->second.date << " Current Balance: " << newAccount.checkBalance() << "\n";


        //load snapshot
        logger.logSnapshot(bars[feeds[0].ticker].date, tempBarLog, newAccount.checkBalance(), newAccount.accountValue(currPrices), newAccount.returnPositions());
        
        //next bar for all tickers
        for(auto& tempFeed : feeds){
            tempFeed.nextBar();
        }
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
    double returns = calculate.totalReturn(initBalance, currPrices);
    double cagr = calculate.cagr(initBalance, currPrices, 10);
    
    std::cout<<"Total return: " << returns << std::endl;
    std::cout<<"CAGR: " << cagr << std::endl;

    //checking logger
    //date : 2015-01-12 00:00:00-05:00
    //logger.printSnapshot("2015-04-20 00:00:00-05:00");

    std::string filename = "../data/test.csv";
    logger.exportCSV(filename);
    
}