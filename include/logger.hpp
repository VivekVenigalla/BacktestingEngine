#pragma once
#include "./structures.hpp"
#include "./performanceEval.hpp"
#include <filesystem>

namespace fs = std::filesystem;

class Logger{
    public:
        History fullHistory;
        //in order to allow for faster access of a specific date, we use a lookup map that associates each date with its appropraite index in a unordered map, allowing for fast lookup of a specific entry

        void logSnapshot(std::string date, std::unordered_map<std::string, Bar> bar, double balance, double equity, std::unordered_map<std::string, Position> positions, double drawdown);
    //exports all data in a folder with two csv files(bar and trade history) and 1 json file(metrics data)
        void exportData(std::string& simID, Metrics& calculator, std::unordered_map<long int, Trade>& historyRef, std::unordered_map<std::string, double>& currPrices, double& initBalance, double& cagrLength);
        
        void exportCSV(fs::path filepath, std::string filename);
        
        void exportCSVTrade(fs::path filepath, std::string filename, std::unordered_map<long int, Trade>& historyRef);
        
        void exportJSON(fs::path filepath, std::string filename, Metrics& calculator, std::string& simID, std::unordered_map<std::string, double>& currPrices, double& initBalance, double& cagrLength, std::unordered_map<long int, Trade>& historyRef);
        
        void printAllSnapshots();
        void printSnapshot(std::string date);
    private:
        std::unordered_map<std::string, int> lookupMap;

};