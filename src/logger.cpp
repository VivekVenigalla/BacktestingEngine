#include "../include/logger.hpp"
#include "../include/monteCarlo.hpp"
#include <fstream>
#include <iomanip>
#include <memory>
#include <algorithm>

#include "nlohmann/json.hpp"
#include "projectPaths.hpp"

using json = nlohmann::json;


void Logger::logSnapshot(std::string date, std::unordered_map<std::string, Bar> bar, double balance, double equity, std::unordered_map<std::string, Position> pos, double drawdown){
    //insert the snapshot into the structure
    fullHistory.dates.push_back(date);
    fullHistory.bars.push_back(bar);
    fullHistory.balances.push_back(balance);
    fullHistory.totalEquity.push_back(equity);
    fullHistory.positions.push_back(pos);
    fullHistory.drawDown.push_back(drawdown);

    //create a lookup entry for fast access
    lookupMap[date] = fullHistory.dates.size()-1;


}

void Logger::printSnapshot(std::string date){
    fullHistory.print_with_date(lookupMap[date]);
}

void Logger::printAllSnapshots(){
    for(std::string date : fullHistory.dates){
        fullHistory.print_with_date(lookupMap[date]);
    }
}

//filename requires a proper directory
void Logger::exportCSV(fs::path filepath, std::string filename){
    //ofstream("output file stream") opens a file for WRITING - the
    //opposite of ifstream, which csvParser.cpp uses for reading
    //ofstream only allows writing to files
    std::ofstream file(filepath);

    //"if(!file)" checks whether the file stream is in a good, usable
    //state - file streams can be checked like a bool this way, and it
    //comes back false if the file couldn't be opened/created(e.g the
    //folder doesn't exist or there's no write permission)
    if(!file){
        std::cerr<<"File " << filename << " unable to be created. Terminating export..." << std::endl;
        return;
    }
    else{
        //only focus on these values shown below, will look into position and bar values later
        file << "Date,Balance,Equity,DrawDown,Ticker,BarOpen,BarHigh,BarLow,BarClose,BarVolume,Quantity,AEP\n";

        //loop through the vectors and input them one by one
        //for each ticker the csv has another row with the same date
        for(int i =0; i < fullHistory.dates.size(); i++){
            for(const auto& [key, value] : fullHistory.bars[i]){
                file << fullHistory.dates[i] << ","
                << fullHistory.balances[i] << ","
                << fullHistory.totalEquity[i] << ","
                << fullHistory.drawDown[i] << ","
                << key << ","
                << value.open << ","
                << value.high << ","
                << value.low << ","
                << value.close << ","
                << value.volume << ","
                << fullHistory.positions[i][key].quantity << ","
                << fullHistory.positions[i][key].average_entry_price << "\n";
            }
        }

        file.close();

        std::cout<<"CSV File " << filename << " created..." << std::endl;
    }
}

void Logger::exportJSON(fs::path filepath, std::string filename, Metrics& calculator, std::string& simID, std::unordered_map<std::string, double>& currPrices, double& initBalance, double& cagrLength, std::unordered_map<long int, Trade>& historyRef, double periodsPerYear){
    //obtain all metrics
    double totalReturns = calculator.totalReturn(initBalance, currPrices);
    double cagr = calculator.cagr(initBalance, currPrices, cagrLength);
    int tradeNum = historyRef.size();
    int successfulTrade = 0;

    for(auto& [key, value] : historyRef){
        if(value.filled){
            successfulTrade++;
        }
    }
    int unsucTrade = tradeNum - successfulTrade;

    //additional performance stats computed from the bar-by-bar history already
    //recorded via logSnapshot -- no extra data plumbing needed
    double maxDD = calculator.maxDrawdown();
    std::vector<double> returns = Metrics::returnsFromEquityCurve(fullHistory.totalEquity);
    double sharpe = calculator.sharpeRatio(returns, 0.0, periodsPerYear);
    double sortino = calculator.sortinoRatio(returns, 0.0, periodsPerYear);

    //benchmarkReturn needs the first and last logged bar's price - guarded
    //by these empty-checks since a simulation with zero logged bars(or an
    //empty first snapshot) would otherwise crash trying to read
    //.front()/.begin() on an empty container
    double benchmark = 0.0;
    if(!fullHistory.bars.empty() && !fullHistory.bars.front().empty()){
        std::string primaryTicker = fullHistory.bars.front().begin()->first;
        double startPrice = fullHistory.bars.front().at(primaryTicker).close;
        double endPrice = fullHistory.bars.back().at(primaryTicker).close;
        benchmark = calculator.benchmarkReturn(startPrice, endPrice);
    }


    //open file
    std::ofstream file(filepath);

    if(!file){
        std::cerr<<"File " << filename << " unable to be created. Terminating export..." << std::endl;
        return;
    }
    else{
        json metric;
        metric["simID"] = simID;
        metric["totalReturns"] = totalReturns;
        metric["CAGR"] = cagr;
        metric["MaxDrawdown"] = maxDD;
        metric["SharpeRatio"] = sharpe;
        metric["SortinoRatio"] = sortino;
        metric["BenchmarkReturn"] = benchmark;
        metric["Trade_records"]["Number_of_trades"] = tradeNum;
        metric["Trade_records"]["Successful_trades"] = successfulTrade;
        metric["Trade_records"]["Unsuccessful_trades"] = unsucTrade;
        metric["Trade_records"]["Win_rate"] = calculator.winRate();
        metric["Trade_records"]["Profit_factor"] = calculator.profitFactor();

        //metric.dump(4) turns the json object into text, indented 4 spaces
        //per level so the file is human-readable rather than one giant
        //unbroken line
        //dump the json object in the file
        file << metric.dump(4);
        file.close();
    }
    std::cout<<"JSON File " << filename << " created..." << std::endl;

}

void Logger::exportCSVTrade(fs::path filepath, std::string filename, std::unordered_map<long int, Trade>& historyRef){
    //ofstream only allows writing to files
    std::ofstream file(filepath);

    if(!file){
        std::cerr<<"File " << filename << " unable to be created. Terminating export..." << std::endl;
        return;
    }
    else{
        /*
        std::string ticker;
    double execPrice;
    std::string type;
    bool side;
    long quantity;
    double checkPrice;
    //figure out the commmision calculation
    double commision;
    bool filled;
    std::string status;
    double currBalance;
    */
        //only focus on these values shown below, will look into position and bar values later
        //RealizedPnL/LimitPrice/Date appended at the END of the header
        //(rather than interleaved with the older columns) so any existing
        //code that expects the original 11-column layout(by position) is
        //disturbed as little as possible
        file << "TradeID,TickerID,ExecPrice,Type,Side,Quantity,CheckPrice,Commission,Filled,Status,CurrentBalance,RealizedPnL,LimitPrice,Date\n";

        //loop through the vectors and input them one by one
        //for each ticker the csv has another row with the same date
        for(auto& [key, value] : historyRef){
            file << key << ","
            << value.ticker << ","
            << value.execPrice << ","
            << value.type << ","
            << value.side << ","
            << value.quantity << ","
            << value.checkPrice << ","
            << value.commision << ","
            << (value.filled ? "true" : "false") << "," //conditional operator converts bool to string
            << value.status << ","
            << value.currBalance << ","
            << value.realizedPnL << ","
            << value.limitPrice << ","
            << value.date << "\n";
        }

        file.close();

        std::cout<<"CSV Trade File " << filename << " created..." << std::endl;
    }
}

void Logger::exportMonteCarloJSON(fs::path filepath, std::string filename, std::string& simID, double initBalance, int numRuns, bool hasSeed, unsigned int seed){
    //same primitive exportJSON already uses to turn this run's bar-by-bar
    //equity curve into a period-return series - Monte Carlo resamples
    //THOSE returns, it never touches Account/Broker or re-runs the sim
    std::vector<double> returns = Metrics::returnsFromEquityCurve(fullHistory.totalEquity);

    //MonteCarloSimulator has no default constructor(rng must be seeded up
    //front), so a plain local variable can't be conditionally reassigned -
    //a unique_ptr lets both branches below construct it in place
    std::unique_ptr<MonteCarloSimulator> simulator;
    if(hasSeed){
        simulator = std::make_unique<MonteCarloSimulator>(returns, seed);
    }
    else{
        simulator = std::make_unique<MonteCarloSimulator>(returns);
    }

    std::vector<double> finalEquities = simulator->bootstrapResample(numRuns, initBalance);
    std::vector<double> maxDrawdowns = simulator->shuffledOrderResample(numRuns, initBalance);

    //10 equal-width buckets spanning the observed final-equity range - a
    //quick histogram of "how many of the numRuns outcomes landed in each
    //slice", useful for a GUI plot later without shipping every raw sample
    const int bucketCount = 10;
    double minEquity = *std::min_element(finalEquities.begin(), finalEquities.end());
    double maxEquity = *std::max_element(finalEquities.begin(), finalEquities.end());
    std::vector<int> bucketCounts(bucketCount, 0);
    std::vector<double> bucketEdges;
    double bucketWidth = (maxEquity - minEquity) / static_cast<double>(bucketCount);

    for(int i = 0; i <= bucketCount; i++){
        bucketEdges.push_back(minEquity + bucketWidth * i);
    }

    for(double equity : finalEquities){
        //every run's final equity lands in exactly one bucket - if the
        //whole range collapses to a single value(bucketWidth == 0, e.g a
        //strategy that never traded), everything just falls into bucket 0
        int bucketIndex = 0;
        if(bucketWidth > 0.0){
            bucketIndex = static_cast<int>((equity - minEquity) / bucketWidth);
            //the single run that lands exactly on maxEquity would compute
            //to bucketCount(one past the last valid index) - clamp it into
            //the final bucket instead
            if(bucketIndex >= bucketCount){
                bucketIndex = bucketCount - 1;
            }
        }
        bucketCounts[bucketIndex]++;
    }

    std::ofstream file(filepath);

    if(!file){
        std::cerr<<"File " << filename << " unable to be created. Terminating export..." << std::endl;
        return;
    }

    json result;
    result["simID"] = simID;
    result["runs"] = numRuns;
    result["seed"] = hasSeed ? json(seed) : json(nullptr);
    result["bootstrapFinalEquity"]["p5"] = MonteCarloSimulator::percentile(finalEquities, 5.0);
    result["bootstrapFinalEquity"]["p50"] = MonteCarloSimulator::percentile(finalEquities, 50.0);
    result["bootstrapFinalEquity"]["p95"] = MonteCarloSimulator::percentile(finalEquities, 95.0);
    result["bootstrapFinalEquity"]["histogram"]["bucketEdges"] = bucketEdges;
    result["bootstrapFinalEquity"]["histogram"]["counts"] = bucketCounts;
    result["shuffledOrderMaxDrawdown"]["p5"] = MonteCarloSimulator::percentile(maxDrawdowns, 5.0);
    result["shuffledOrderMaxDrawdown"]["p50"] = MonteCarloSimulator::percentile(maxDrawdowns, 50.0);
    result["shuffledOrderMaxDrawdown"]["p95"] = MonteCarloSimulator::percentile(maxDrawdowns, 95.0);

    file << result.dump(4);
    file.close();

    std::cout<<"JSON File " << filename << " created..." << std::endl;
}

void Logger::exportData(std::string& simID, Metrics& calculator, std::unordered_map<long int, Trade>& historyRef, std::unordered_map<std::string, double>& currPrices, double& initBalance, double& cagrLength, std::string batchID, double periodsPerYear, bool monteCarloEnabled, int monteCarloRuns, bool monteCarloHasSeed, unsigned int monteCarloSeed){

    //path to the output folder
    //fs::path(...) / "output" builds a path by joining pieces together with
    //the operating system's own path separator, so this works correctly
    //whether it runs on mac/linux(a/b/c) or windows(a\b\c)
    fs::path baseDir = fs::path(PROJECT_SOURCE_DIR) / "output";
    baseDir = baseDir / batchID;

    //this is a temporary path with the simID
    fs::path targetFolder = baseDir / simID;

    //create the unique folder path by checking which name is available
    //keeps appending _1, _2, _3... until it finds a folder name that
    //doesn't already exist, so re-running the same sim never overwrites a
    //previous run's output
    int counter = 1;
    std::string uniqueID = simID;

    while(fs::exists(targetFolder)){
        uniqueID = simID + "_" + std::to_string(counter);
        targetFolder = baseDir / uniqueID;
        counter++;
    }
    //when the while loop breaks the folder path is now unique

    //create the directory
    fs::create_directories(targetFolder);
    exportDir = targetFolder;

    //export csv data
    std::string csvFile = "dynamicData.csv";

    fs::path csvPath = targetFolder / csvFile;
    exportCSV(csvPath, csvFile);

    //export csv trade data
    csvFile = "tradeData.csv";

    csvPath = targetFolder / csvFile;
    exportCSVTrade(csvPath, csvFile, historyRef);

    //export json data
    std::string jsonFile = "metricData.json";

    fs::path jsonPath = targetFolder / jsonFile;
    exportJSON(jsonPath, jsonFile, calculator, simID, currPrices, initBalance, cagrLength, historyRef, periodsPerYear);

    //optional 4th output file - only written when the sim's batch config
    //explicitly opted in via a "monte_carlo": {"enabled": true, ...} block.
    //the three exports above are completely unaffected either way
    if(monteCarloEnabled){
        std::string mcFile = "monteCarloResults.json";
        fs::path mcPath = targetFolder / mcFile;
        exportMonteCarloJSON(mcPath, mcFile, simID, initBalance, monteCarloRuns, monteCarloHasSeed, monteCarloSeed);
    }

    std::cout << "All files successfully created in directory: " << targetFolder.string() << "\n";

}
