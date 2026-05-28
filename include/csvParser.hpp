#include <vector>
#include <string>
#include <structures.hpp>

//this current parser will only focus on the one file in the data folder. Later the code will implement a file selection system for data parsing
class Parser{
    public:
        //open the file
        //loop through the contents(making sure to skip the first row)
        std::vector<Bar> parse();
    private:
        //data path to the only file in this folder for now
        std::string DATA_PATH = "../data/AAPL_interval_set.csv";
        std::string row;
        std::vector<Bar> data;
};