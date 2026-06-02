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