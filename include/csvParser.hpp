#pragma once
#include <vector>
#include <string>
#include <structures.hpp>
#include <map>
//this current parser will only focus on the one file in the data folder. Later the code will implement a file selection system for data parsing
class Parser{
    public:
        //open the file
        //loop through the contents(making sure to skip the first row)
        //previous implementation => std::vector<Bar> parse();
        std::map<std::string, Bar> parse(std::string ticker);
        std::map<std::string, Bar> parse();
    private:
        //data path to the only file in this folder for now
        std::string DATA_PATH = "../data/AAPL_1d.csv";
        //std::vector<Bar> data;
        //map implementation
        //utilizing a map allows for easy location of specific Bars since you can associate each key with a date
        //data.first => date of Bar data.second => ohlcv
};