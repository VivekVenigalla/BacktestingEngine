#pragma once
#include <structures.hpp>
#include "./account.hpp"
//includes map and string

//we include this file since the broker connects to the account to fulfill transactions

//types of orders
//simple market buy or sell
//limit order(buy or sell once price reaches a target)

//each order is correalated with an id that is auto generated

class Broker{
    //initialization of class: need account to connect to. Will also need a connection with a file that can feed in price data
    //for the time being assume that the price will be directly inputted into the functions
    public:
        //the broker is connected to the account with no direct modifier so it can use its methods
        //since the broker has a reference to the currBar in main.cpp, there is no need for a function to assign the bar every main iteration
        Broker(Account& account, Bar& connectBar);

        //the following three functions are to be used by the stratgey.hpp/cpp
        //the strategy will create the Order struct and send it to be processed by the Broker.
        //will most likely be a int to return the id of the Order for future reference of the strategy)
        //most likely the strategy will not modify orders but this is just in case

        int createOrder(Order newOrder);


        //IMPORTANT NOTE:Orders can be created but cannot be changed or deleted

        //main functions to be used by central governing script

        //main functions to be used by central governing script
        void checkLoop();
        

        //getter methods
        std::unordered_map<long int, Trade>& returnHistory();
        std::unordered_map<long int, Order>& returnOrders();
        
        
    private:

        //need a vector of orders that will be accessed by the Broker every Bar to execute limit or market
        //orders.first => order id(unique)
        int tempID = 1;
        Account& user;
        //order history will stay with strategy so the references can stay intact
        //since market orders are prioritized first, they are in a seperate unordered map in order to iterate through them first for processing
        std::unordered_map<long int, Order> orders;
        std::unordered_map<long int, Trade> history;
        Bar& currBar;

        bool checkOrder(Order& check);

        bool checkOrderLimitAndStop(Order check);

        void processOrder(int id, Order order);
        
        
};