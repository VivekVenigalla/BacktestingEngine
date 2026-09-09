//this is the program's entry point - the one function every c++ program
//must have, which the operating system calls first when you run ./runny.
//everything else in the codebase is just building blocks; this file is
//where they all actually get wired together and run for real
//
//the high-level flow: read a batch config json -> build shared Data feeds/
//Accounts/Brokers from it -> for each simulation listed in the config,
//build a Strategy + SimulationRunner and drive it to completion -> export
//results. see simulationRunner.cpp's step() for what happens on each
//individual bar within one simulation
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
#include "../include/simulationRunner.hpp"
#include "../include/logger.hpp"
#include "nlohmann/json.hpp"
#include "projectPaths.hpp"
#include <filesystem>

namespace fs = std::filesystem;

//namspace of json instead of nlohmann::json for easy use
using json = nlohmann::json;

//no using namespace std to ensure readability

//argc(argument count) and argv(argument vector) are how command-line
//arguments reach a c++ program - argv[0] is always the program's own name/
//path, argv[1] onward are whatever the user typed after it. running
//"./runny myconfig.json" means argc==2, argv[0]=="./runny", argv[1]==
//"myconfig.json"
int main(int argc, char* argv[]) {

    //there will be one argument that is the JSON Path

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path-to-batch-config.json>" << std::endl;
        return 1;
    }

    std::string argument = argv[1];
    std::string JSON_PATH =  argument;
    //connect to the file
    std::ifstream file(JSON_PATH);

    //bug tracking
    if (!file.is_open()) {
        std::cerr << "Failed to open config file at: " << JSON_PATH << std::endl;
        return 1;
    }
    //create json object and move the file to the json object allowing for parsing
    //"file >> config" reads the whole file and parses it as json in one
    //step - the same >> operator you'd use to read a number or word from
    //std::cin, just overloaded by the json library to understand json text
    json config;
    file >> config;

    //obtain batch id
    //config["simulation_metadata"]["batch_id"] digs into nested json
    //objects the same way you'd index nested dictionaries in most
    //languages - .get<std::string>() then converts that json value into a
    //real std::string
    std::string batchID = config["simulation_metadata"]["batch_id"].get<std::string>();

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
    //"for(auto& feed : config["data_feeds"])" walks every entry in the
    //config's data_feeds json array, one at a time - same range-based for
    //loop syntax used elsewhere in this codebase for vectors/maps, just
    //over a json array here instead
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
    //.emplace(key, value) constructs the Data object directly inside the
    //map, in place - this avoids building a separate Data object first and
    //then copying it in, which matters here since building one already
    //means parsing an entire csv file
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
        allAccounts.emplace(acctID, Account{account["initial_balance"].get<double>(), tempTickers, acctID});
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
            broker["slippage_rate"].get<double>(),
            brokerID
        ));
    }

    //create the logger, metrics, and strategies maps
    //each strategy gets its own logger and calculator
    std::unordered_map<std::string, Logger> loggers;
    std::unordered_map<std::string, Metrics> calculators;
    //unique ptr allows for polymorphism
    //storing std::unique_ptr<Strategy> here(rather than a plain Strategy)
    //is what lets this one map hold a mix of different concrete strategy
    //types(smaCross, bollBand, donChannel...) side by side, since they all
    //inherit from the same Strategy base class
    std::unordered_map<std::string, std::unique_ptr<Strategy>> strategies;

    //calculator metrics
    double initBalance;
    double returns;
    double cagr;

    //create batch output directory
    fs::path base_dir = fs::path(PROJECT_SOURCE_DIR) / "output";
    base_dir = base_dir / batchID;
    if(fs::create_directory(base_dir)){
        std::cout<<"Batch output directory created at: " << base_dir << std::endl;
    }

    //core loop
    //this is the main loop - it runs once per simulation listed in the
    //config's "simulations" array, building a fresh Strategy+Logger+Metrics
    //trio for each one and driving it to completion before moving to the next
    for (auto& sim : config["simulations"]) {
        //get important variables here
        std::string simID = sim["id"].get<std::string>();
        std::string stratType = sim["strategy"].get<std::string>();
        std::string acctLink = sim["account_link"].get<std::string>();
        std::string brokerLink = sim["broker_link"].get<std::string>();
        //sim.value<bool>("run_all_by_default", true) reads that key as a
        //bool if present, or just uses true if the key is missing entirely
        //- same "give me this value or a fallback" idea as
        //createStrat.cpp's config.value("position_size_pct", 0.2)
        bool runAll = sim.value<bool>("run_all_by_default", true);

        json accountConfig;
        json brokerConfig;
        for (const auto& acc : config["account"]) {
            if (acc.contains("id") && acc["id"] == acctLink) {
                accountConfig = acc;
                break;
            }
        }

        for (const auto& bro : config["broker"]) {
            if (bro.contains("id") && bro["id"] == brokerLink) {
                brokerConfig = bro;
                break;
            }
        }


        bool accountReset = accountConfig.value<bool>("reset", true);
        bool brokerReset = brokerConfig.value<bool>("reset", true);

        //.at() bypasses standard constructor requirements which gives an error
        //.at(key) looks up a map entry and THROWS if the key doesn't
        //exist, unlike operator[](used elsewhere in this file), which would
        //silently create a new blank entry instead - .at() is used here on
        //purpose, since a missing account_link/broker_link is a real
        //config mistake worth failing loudly on, not silently ignoring
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
        //"sim.contains(...) ? sim["parameters"] : json::object()" is the
        //ternary(conditional) operator - shorthand for an if/else that
        //picks one of two values: use the sim's own "parameters" if it has
        //any, otherwise fall back to an empty json object
        json stratParams = sim.contains("parameters") ? sim["parameters"] : json::object();
        strategies.emplace(simID, StrategyFactory::create(
            tempBroker, tempAccount, tempBars, historyRef, tempTickers, stratType, stratParams
        ));

        auto& strategy = strategies[simID];
        strategy->init();

        std::unordered_map<std::string, double> currPrices;
        std::string primaryID = feedIDs[0];

        //obtain cgar length
        json primaryFeedConfig = config.contains("data_feeds") ? config["data_feeds"][0] : json::object();

        double cagrLength = primaryFeedConfig["cagr_length"];

        //size_t is a storage method for the size of objects
        size_t totalHistoricalBarsCount = feeds[primaryID].totalBars();

        //create runner obect
        SimulationRunner runner(
            simID, tempAccount, tempBroker, strategy, tempLogger, calculator,
            feeds, bars, tempBars, currPrices, feedIDs, initBalance, cagrLength, totalHistoricalBarsCount, batchID
        );

        //if config says so run the entire simulation
        if(runAll){
            runner.runAll();
        }
        else{
            //this branch is an interactive text menu - std::cin >> command
            //blocks and waits for you to type something in the terminal
            //and hit enter, letting you step through the simulation bar by
            //bar by hand instead of running it all at once
            std::string command;
            while(!runner.getIsFinished()){
                std::cout << "Controls: 's'=Step, 'n'=N Steps, 'd'=Run To Date, 'r'=Run All, 'q'=Skip -> Option: ";
                std::cin >> command;
                //get the command and check based on the controls
                if(command == "s") {
                    runner.step();
                }
                else if(command == "n") {
                    size_t stepsCount;
                    std::cout << "Enter number of bars to process: ";
                    std::cin >> stepsCount;
                    runner.runSteps(stepsCount);
                }
                else if(command == "d") {
                    std::string targetDateString;
                    std::cout << "Enter target window limit string (YYYY-MM-DD): ";
                    std::cin >> targetDateString;
                    runner.runToDate(targetDateString);
                }
                else if(command == "r") {
                    runner.runAll();
                }
                else if(command == "q") {
                    break;
                }
                else{
                    std::cout << "Invalid input. Please try again" << "\n";
                }
            }


        }
        //reset account and broker
        if(accountReset){
            tempAccount.reset();
        }
        if(brokerReset){
            tempBroker.reset();
        }
    }


    //finish :)
    std::cout << "\nSimulations finished successfully." << std::endl;
    return 0;
}
