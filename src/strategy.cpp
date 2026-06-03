#include "../include/strategy.hpp"

Strategy::Strategy(Broker& b, Account& u, Bar& cB, std::unordered_map<long int, Trade>& history) : broker(b), user(u), connectBar(cB), tradeHistory(history){
} 

void Strategy::loadBar(){
    Bar temp = connectBar;
    barHistory.push_back(temp);
}
