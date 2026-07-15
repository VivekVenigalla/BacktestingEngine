#pragma once
#include <structures.hpp>
#include <csvParser.hpp>

class Data{

    public:
        Data();
        Data(std::string tick);
        void nextBar();
        Bar& getBar();
        bool hasMoreData();
        void reset();
        std::string ticker;
    private:
        std::map<std::string, Bar>::iterator currBar;
        std::map<std::string, Bar> data;
        Parser parser;
        std::map<std::string, Bar> parse();
        
};