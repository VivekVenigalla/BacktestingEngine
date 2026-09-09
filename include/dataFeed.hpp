#pragma once
#include <structures.hpp>
#include <csvParser.hpp>

//Data streams one ticker's bars out one at a time, in date order, standing
//in for "live" market data during a backtest - the simulation calls
//getBar()/nextBar() over and over to walk forward through history bar by
//bar, instead of the strategy ever seeing the whole price history at once
class Data{

    public:
        Data();
        Data(std::string id, std::string tick, std::string path);
        void nextBar();
        Bar& getBar();
        bool hasMoreData();
        //a function body can be written right inside the class definition
        //like this instead of in the .cpp file - this is called an inline
        //function, usually reserved for very short one-line functions like
        //this getter. size_t is an unsigned integer type used across the
        //standard library for sizes/counts(here, data.size() - how many
        //bars this feed holds in total)
        size_t totalBars() const {return data.size();}
        void reset();
        std::string ticker;
        std::string ID;
        std::string PATH;
    private:
        //an iterator is like a movable bookmark into a container - storing
        //one as a member field(rather than a local variable) is what lets
        //this class remember "which bar are we currently on" across
        //separate calls to nextBar()/getBar(), instead of forgetting its
        //position the moment each function returns
        std::map<std::string, Bar>::iterator currBar;
        std::map<std::string, Bar> data;
        //Parser parser; is composition("Data HAS-A Parser") rather than
        //inheritance("Data IS-A Parser") - Data just owns one Parser
        //object internally and calls its parse() method to fill "data" up
        Parser parser;
        std::map<std::string, Bar> parse();

};
