#include "../include/logger.hpp"

void Logger::logSnapshot(std::string date, std::unordered_map<std::string, Bar> bar, double balance, double equity, std::unordered_map<std::string, Position> pos){
    //insert the snapshot into the structure
    fullHistory.dates.push_back(date);
    fullHistory.bars.push_back(bar);
    fullHistory.balances.push_back(balance);
    fullHistory.totalEquity.push_back(equity);
    fullHistory.positions.push_back(pos);

    //create a lookup entry for fast access
    lookupMap[date] = fullHistory.dates.size()-1;
    
    
}

void Logger::printSnapshot(std::string date){
    fullHistory.print_with_date(lookupMap[date]);
}