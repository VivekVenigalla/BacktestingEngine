#include "../include/strategy.hpp"

Strategy::Strategy(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::string symbol) : broker(b), user(u), connectBars(cBs), tradeHistory(history), ticker(symbol){
} 

void Strategy::loadBar(){
    //this implementation works best for one stock
    Bar temp = connectBars.begin()->second;
    barHistory.push_back(temp);
}
