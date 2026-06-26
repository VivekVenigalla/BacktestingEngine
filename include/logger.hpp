#pragma once
#include "./structures.hpp"

class Logger{
    public:
        History fullHistory;
        //in order to allow for faster access of a specific date, we use a lookup map that associates each date with its appropraite index in a unordered map, allowing for fast lookup of a specific entry

        void logSnapshot(std::string date, std::unordered_map<std::string, Bar> bar, double balance, double equity, std::unordered_map<std::string, Position> positions);
    //exports the history as a csv file
        void exportCSV(std::string fileName);

        void printSnapshot(std::string date);
    private:
        std::unordered_map<std::string, int> lookupMap;

};