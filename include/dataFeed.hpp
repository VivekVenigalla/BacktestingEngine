#pragma once
#include <structures.hpp>
#include <csvParser.hpp>

class Data{

    public:
        Data();
        Data(std::string id, std::string tick, std::string path);
        void nextBar();
        Bar& getBar();
        bool hasMoreData();
        void reset();
        std::string ticker;
        std::string ID;
        std::string PATH;
    private:
        std::map<std::string, Bar>::iterator currBar;
        std::map<std::string, Bar> data;
        Parser parser;
        std::map<std::string, Bar> parse();
        
};