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
#include "../include/strategies/smaCross.hpp"
#include "../include/strategies/bollBand.hpp"
#include "../include/strategies/donChannel.hpp"
#include "../include/performanceEval.hpp"
#include "../include/logger.hpp"
#include "nlohmann/json.hpp"

//namspace of json instead of nlohmann::json for easy use
using json = nlohmann::json;

//no using namespace std to ensure readability

int main() {
    
    //JSON_PATH will be altered when connected to the GUI
    std::string JSON_PATH = "../config/simConfig.json";
    //connect to the file
    std::ifstream file(JSON_PATH);

    //bug tracking
    if (!file.is_open()) {
        std::cerr << "Failed to open config file at: " << JSON_PATH << std::endl;
        return 1;
    }
    //create json object and move the file to the json object allowing for parsing
    json config;
    file >> config;

    //tickers and paths for the feed objects

    //the key is the ticker id, not to be confused with the ticker(which is not used in the logic but only for quick reference)
    //ticker id includes the precision and timeframe
    //the ticker id allows access to all of the data info
    std::unordered_map<std::string, std::string> tickers;
    //just includes the ticker ids
    std::vector<std::string> tempTickers;
    //includes the paths to each of the feeds for parsing
    std::unordered_map<std::string, std::string> paths;

    //iterate over the feeds
    for (auto& feed : config["data_feeds"]) {
        //temporary feedID
        std::string feedID = feed["id"].get<std::string>();
        //fill up the variables above
        tickers[feedID] = feed["ticker"].get<std::string>();
        tempTickers.push_back(feedID);
        paths[feedID] = feed["csv_filepath"].get<std::string>();
    }

    //create the feeds object
    //each of the feed contains all of the bar data, which is used to drip the strategy the bar to simulate real time trading
    std::unordered_map<std::string, Data> feeds;
    //reserve puts away space for the amount of tickers, useful for larger simulations
    feeds.reserve(tickers.size()); 

    //iterate over the tickers map and create each feed
    //creating the feed will automatically parse the data given the paths
    for (const auto& [key, value] : tickers) {
        feeds.emplace(key, Data{key, value, paths[key]});
    }

    //this variable holds the bars that will drip into the strategies one by one
    //iterate over all of the feeds and initialize the bars with the first member of each feed
    std::unordered_map<std::string, Bar> bars;
    for (auto& [key, value] : feeds) {
        bars[key] = value.getBar();
    }

    //since each simulation may require a different account, create the accounts using the json file
    std::unordered_map<std::string, Account> allAccounts;
    for (auto& account : config["account"]) {
        //acctID is used to link the accounts with the broker and strategy
        std::string acctID = account["id"].get<std::string>();
        //create the account with the emplace function
        allAccounts.emplace(acctID, Account{account["initial_balance"].get<double>(), tempTickers});
    }

    //create the brokers
    std::unordered_map<std::string, Broker> allBrokers;
    for (auto& broker : config["broker"]) {
        std::string brokerID = broker["id"].get<std::string>();
        std::string acctLink = broker["account_link"].get<std::string>();
        
        //broker id is not needed in the class
        allBrokers.emplace(brokerID, Broker(
            //brokerID,
            allAccounts.at(acctLink), 
            bars, 
            broker["commission_rate"].get<double>(), 
            broker["slippage_rate"].get<double>()
        ));
    }

    //create the logger, metrics, and strategies maps
    //each strategy gets its own logger and calculator
    std::unordered_map<std::string, Logger> loggers;
    std::unordered_map<std::string, Metrics> calculators;
    //unique ptr allows for polymorphism
    std::unordered_map<std::string, std::unique_ptr<Strategy>> strategies;

    //calculator metrics
    double initBalance;
    double returns;
    double cagr;

    //core loop
    for (auto& sim : config["simulations"]) {
        //get important variables here
        std::string simID = sim["id"].get<std::string>();
        std::string stratType = sim["strategy"].get<std::string>();
        std::string acctLink = sim["account_link"].get<std::string>();
        std::string brokerLink = sim["broker_link"].get<std::string>();


        //.at() bypasses standard constructor requirements which gives an error
        Account& tempAccount = allAccounts.at(sim["account_link"].get<std::string>());
        Broker& tempBroker = allBrokers.at(sim["broker_link"].get<std::string>());

        //get initial balances
        initBalance = tempAccount.checkBalance();

        //crete logger
        loggers.emplace(simID, Logger{});
        Logger& tempLogger = loggers[simID];

        //get the important feeds and bars
        std::unordered_map<std::string, Bar> tempBars;
        std::vector<std::string> feedIDs;
        
        for (auto& feed : sim["feeds"]) {
            std::string feedID = feed.get<std::string>();
            feedIDs.push_back(feedID);
            feeds[feedID].reset(); //reset the feeds in case they were used earlier
            tempBars[feedID] = feeds[feedID].getBar();
        }

        std::unordered_map<long int, Trade>& historyRef = tempBroker.returnHistory();

        //create calculator
        calculators.emplace(simID, Metrics{tempAccount, historyRef, tempTickers});
        Metrics& calculator = calculators.at(simID);

        //create strategy
        json stratParams = sim.contains("parameters") ? sim["parameters"] : json::object();
        strategies.emplace(simID, StrategyFactory::create(
            tempBroker, tempAccount, tempBars, historyRef, tempTickers, stratType, stratParams
        ));
        
        auto& strategy = strategies[simID];
        strategy->init();

        std::unordered_map<std::string, double> currPrices;
        std::string primaryID = feedIDs[0];
        //run simulation
        while (feeds[primaryID].hasMoreData()) {
            for (const std::string& id : feedIDs) {
                tempBars[id] = feeds[id].getBar();
                bars[id] = tempBars[id]; 
                currPrices[id] = tempBars[id].close;
            }

            tempBroker.checkLoop();

            strategy->loadBar();
            strategy->runBar();
            
            double value = tempAccount.accountValue(currPrices);
            //command line log for debugging purposes
            std::cout << "Sim[" << simID << "] Date: " << tempBars[primaryID].date 
                      << " | Balance: " << tempAccount.checkBalance() << "| Total Equity: " << value << "\n";

            //log important details for plotter => Will improve later
            tempLogger.logSnapshot(
                tempBars[primaryID].date, 
                tempBars, 
                tempAccount.checkBalance(), 
                value, 
                tempAccount.returnPositions()
            );
            
            //iterate to the next bar for all used feeds
            for (const auto& feedID : feedIDs) {
                feeds[feedID].nextBar();
            }

        }
        std::cout << "Simulation [" << simID << "] finished" << "\n";

        //create the metric report
        returns = calculator.totalReturn(initBalance, currPrices);
        //obtain the timeframe distance for cagr in the configs.json
        cagr = calculator.cagr(initBalance, currPrices, 10);

        std::cout << "Returns: " << returns << "\n";
        std::cout << "CAGR: " << cagr << "\n";


        //export individual csvs for each simulation
        std::string filename = "../data/results_" + simID + ".csv";
        tempLogger.exportCSV(filename);
        std::cout << "Exported results to " << filename << std::endl;
    }
    //finish :)
    std::cout << "\nSimulations finished successfully." << std::endl;
    return 0;
}