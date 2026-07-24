#include "../include/logger.hpp"
#include <fstream>
#include <iomanip>

#include "nlohmann/json.hpp"

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
    //ofstream only allows writing to files
    std::ofstream file(filepath);

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

void Logger::exportJSON(fs::path filepath, std::string filename, Metrics& calculator, std::string& simID, std::unordered_map<std::string, double>& currPrices, double& initBalance, double& cagrLength, std::unordered_map<long int, Trade>& historyRef){
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
        metric["Trade_records"]["Number_of_trades"] = tradeNum;
        metric["Trade_records"]["Successful_trades"] = successfulTrade;
        metric["Trade_records"]["Unsuccessful_trades"] = unsucTrade;

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
        file << "TradeID,TickerID,ExecPrice,Type,Side,Quantity,CheckPrice,Commission,Filled,Status,CurrentBalance\n";
        
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
            << value.currBalance << "\n";
        }

        file.close();

        std::cout<<"CSV Trade File " << filename << " created..." << std::endl;
    }
}

void Logger::exportData(std::string& simID, Metrics& calculator, std::unordered_map<long int, Trade>& historyRef, std::unordered_map<std::string, double>& currPrices, double& initBalance, double& cagrLength, std::string batchID){

    //path to the output folder
    fs::path baseDir = fs::path("/Users/vivekvenigalla/Documents/VV_Active/03_PROJECTS/BacktestingEngine/output");
    baseDir = baseDir / batchID;
    
    //this is a temporary path with the simID
    fs::path targetFolder = baseDir / simID;

    //create the unique folder path by checking which name is available
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
    exportJSON(jsonPath, jsonFile, calculator, simID, currPrices, initBalance, cagrLength, historyRef);

    std::cout << "All files successfully created in directory: " << targetFolder.string() << "\n";

}


