#pragma once
#include <vector>
#include <string>
#include <structures.hpp>
#include <map>
//this current parser will only focus on the one file in the data folder. Later the code will implement a file selection system for data parsing

//Parser reads a csv price file off disk and turns it into Bar objects the
//rest of the engine can use. this is the only place in the codebase that
//deals with file text/parsing - everything downstream just works with Bar
//structs and never sees a csv row directly
class Parser{
    public:
        //open the file
        //loop through the contents(making sure to skip the first row)
        //previous implementation => std::vector<Bar> parse();
        //these are two overloads of the same function name - the one
        //below with no arguments always reads the fixed DATA_PATH file and
        //tags every Bar with a placeholder ticker, while the one above
        //takes a real ticker/path so it can be reused for any file
        std::map<std::string, Bar> parse(std::string ticker, std::string path);
        std::map<std::string, Bar> parse();

    private:
        //data path to the only file in this folder for now
        std::string DATA_PATH = "../data/AAPL_1d.csv";
        //std::vector<Bar> data;
        //map implementation
        //std::map(not unordered_map) is used here specifically because it
        //keeps its keys sorted - since the key is the date string, iterating
        //this map naturally walks through bars in chronological order,
        //which is exactly what a bar-by-bar simulation needs
        //utilizing a map allows for easy location of specific Bars since you can associate each key with a date
        //data.first => date of Bar data.second => ohlcv
};
