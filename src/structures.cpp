#include "../include/structures.hpp"


void Bar::print() const{
    std::cout<<ticker << " @ " << date << " :" << std::endl;
    std::cout<<"Open = " << open << std::endl;
    std::cout<<"High = " << high << std::endl;
    std::cout<<"Low = " << low << std::endl;
    std::cout<<"Close = " << close << std::endl;
    std::cout<<"Volume = " << volume << std::endl;
}

void Order::print() const{
    std::cout<<"Ticker = " << ticker << std::endl;
    std::cout<<"Type = " << type << std::endl;
    if(side==0){
        std::cout<<"Side = Buy" << std::endl;
    }
    else{
        std::cout<<"Side = Sell" << std::endl;
    }
    std::cout<<"Quantity = " << quantity << std::endl;
    if(checkPrice >= 0.0){
        std::cout<<"CheckPrice = " <<checkPrice << std::endl;
    }
}

void Position::print() const{
    std::cout<<"Position of Ticker " << ticker << " :" << std::endl;
    std::cout<<"Quantity = " << quantity << std::endl;
    std::cout<<"Average Entry Price = " << average_entry_price << std::endl;
}

void History::print_with_date(int index) const{
    std::cout<<"Date: " << dates[index] << std::endl;

    //iterate over all bar with tickers
    std::unordered_map<std::string, Bar> tempBars = bars[index];
    for(const auto& [key, value] : tempBars){
        value.print();
    }

    std::cout<<"Balance: " << balances[index] << std::endl;
    std::cout<<"Total Equity: " << totalEquity[index] << std::endl;
    std::cout<<"Draw Down: " << drawDown[index] << std::endl;

    std::unordered_map<std::string, Position> tempPos = positions[index];
    for(const auto& [key, value] : tempPos){
        value.print();
    }
}

void Trade::print() const{
    std::cout<<"Ticker = " << ticker << std::endl;
    std::cout<<"Type = " << type << std::endl;
    if(side==0){
        std::cout<<"Side = Buy" << std::endl;
    }
    else{
        std::cout<<"Side = Sell" << std::endl;
    }
    std::cout<<"Quantity = " << quantity << std::endl;
    if(checkPrice >= 0.0){
        std::cout<<"CheckPrice = " <<checkPrice << std::endl;
    }
    std::cout<<"ExecPrice = " <<execPrice << std::endl;
    std::cout<<"Commision = " <<commision << std::endl;
}