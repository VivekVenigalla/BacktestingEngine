#pragma once
#include "./structures.hpp"
#include "./performanceEval.hpp"
#include <filesystem>

//"namespace fs = std::filesystem;" is a namespace alias - std::filesystem
//is the standard library's toolkit for working with file paths/directories
//in a way that works the same on mac/windows/linux. this line just lets the
//rest of the file write the shorter "fs::path" instead of spelling out
//"std::filesystem::path" everywhere
namespace fs = std::filesystem;

//Logger has two jobs: while a simulation is running, logSnapshot() records
//one row of history per bar(into fullHistory); once it's done, exportData()
//writes everything collected so far out to csv/json files on disk
class Logger{
    public:
        History fullHistory;
        //in order to allow for faster access of a specific date, we use a lookup map that associates each date with its appropraite index in a unordered map, allowing for fast lookup of a specific entry

        void logSnapshot(std::string date, std::unordered_map<std::string, Bar> bar, double balance, double equity, std::unordered_map<std::string, Position> positions, double drawdown);
    //exports all data in a folder with two csv files(bar and trade history) and 1 json file(metrics data)
        //periodsPerYear has a default value(= 252.0) - see performanceEval.hpp
        //for what it's for. any caller that doesn't care can just omit it
        void exportData(std::string& simID, Metrics& calculator, std::unordered_map<long int, Trade>& historyRef, std::unordered_map<std::string, double>& currPrices, double& initBalance, double& cagrLength, std::string batchID, double periodsPerYear = 252.0);

        void exportCSV(fs::path filepath, std::string filename);

        void exportCSVTrade(fs::path filepath, std::string filename, std::unordered_map<long int, Trade>& historyRef);

        void exportJSON(fs::path filepath, std::string filename, Metrics& calculator, std::string& simID, std::unordered_map<std::string, double>& currPrices, double& initBalance, double& cagrLength, std::unordered_map<long int, Trade>& historyRef, double periodsPerYear = 252.0);

        void printAllSnapshots();
        void printSnapshot(std::string date);
    private:
        std::unordered_map<std::string, int> lookupMap;

};
