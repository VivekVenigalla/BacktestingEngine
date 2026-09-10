#pragma once
#include <structures.hpp>
#include "./account.hpp"
//includes map and string
//we include the account header file since the broker connects to the account to fulfill transactions

//types of orders
//simple market buy or sell
//limit order(buy or sell once price reaches a target)

//each order is correalated with an id that is auto generated

//Broker is the "exchange" of this engine - a strategy hands it an Order,
//and the broker decides whether/when it actually executes, then tells
//Account to move the cash/shares. the strategy never talks to Account
//directly - everything routes through here, which is what makes slippage,
//commission, and fill rules live in one place instead of being reimplemented
//by every strategy
class Broker{
    //initialization of class: need account to connect to. Will also need a connection with a file that can feed in price data
    //for the time being assume that the price will be directly inputted into the functions
    public:
        //Account& and the unordered_map& below are references, not copies -
        //a reference is like a nickname for a variable that already exists
        //it stores the memory address of the variable and not the actual value itself, but you can use the variable like it has the value
        //useful for having connections of different shared properties
        //writing "Account& account" as a parameter means
        //this Broker doesn't get its own separate Account, it shares the
        //exact same one the caller has - so when Broker changes the
        //balance, the caller's Account object changes too, and there's no
        //cost of copying the whole object on every call
        //the broker is connected to the account with no direct modifier so it can use its methods
        //since the broker has a reference to the currBars in main.cpp, there is no need for a function to assign the bar every main iteration
        Broker(Account& account, std::unordered_map<std::string,Bar>& connectBar);

        Broker(Account& account, std::unordered_map<std::string,Bar>& connectBar, double commision, double slippage, std::string ID);

        //the following three functions are to be used by the stratgey.hpp/cpp
        //the strategy will create the Order struct and send it to be processed by the Broker.
        //will most likely be a int to return the id of the Order for future reference of the strategy)
        //most likely the strategy will not modify orders but this is just in case

        int createOrder(Order newOrder);

        void deleteOrder(int orderID, std::string reason);
        //IMPORTANT NOTE:Orders can be created but cannot be changed or deleted

        //main functions to be used by central governing script

        //main functions to be used by central governing script
        void checkLoop();

        void reset(); //resets order book for other strategies

        //getter methods
        std::unordered_map<long int, Trade>& returnHistory();
        std::unordered_map<long int, Order>& returnOrders();

        std::string id;
    private:

        //need a vector of orders that will be accessed by the Broker every Bar to execute limit or market
        //orders.first => order id(unique)
        int tempID = 1;
        Account& user;
        //standard commision fee will be 1 dollar unless changed in constructor
        double commisionFee = 1.00;
        //standard slippage rate will be 0.05% unless changed in constructor
        double slippageRate = 0.0005;
        //order history will stay with strategy so the references can stay intact
        //since market orders are prioritized first, they are in a seperate unordered map in order to iterate through them first for processing
        std::unordered_map<long int, Order> orders;
        std::unordered_map<long int, Trade> history;
        std::unordered_map<std::string,Bar>& currBars;

        //these three are private - only Broker's own methods above can call
        //them. the strategy is only meant to create/delete orders and read
        //back the results, never to reach in and decide "is this order
        //valid" or "process this order" itself, so those steps are hidden
        bool checkOrder(Order& check);

        bool checkOrderLimitAndStop(Order check);

        void processOrder(int id, Order order);


};
