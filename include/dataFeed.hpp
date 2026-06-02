#pragma once
#include <structures.hpp>
#include <csvParser.hpp>

class Data{

    public:
        Data();

        void nextBar();
        Bar& getBar();
        bool hasMoreData();
        void reset();
    private:
        std::map<std::string, Bar>::iterator currBar;
        std::map<std::string, Bar> data;
        Parser parser;
        std::map<std::string, Bar> parse();
        
};