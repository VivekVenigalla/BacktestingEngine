#include "../../include/strategies/smaCross.hpp"
#include <cmath>
//cmath includes the operations such as floor


smaCross::smaCross(Broker& b, Account& u, Bar& cB, std::unordered_map<long int, Trade>& history, std::string symbol) : broker(b), user(u), connectBar(cB), tradeHistory(history), ticker(symbol), fastLength(50), slowLength(200){
}

smaCross::smaCross(Broker& b, Account& u, Bar& cB, std::unordered_map<long int, Trade>& history, std::string symbol, int fast, int slow) : broker(b), user(u), connectBar(cB), tradeHistory(history), ticker(symbol), fastLength(fast), slowLength(slow){
    if (fastLength >= slowLength) {
        std::cerr << "WARNING: Fast SMA period should be less than Slow SMA period. Adjusting values...\n";
        fastLength = 50;
        slowLength = 200;
    }
}



void smaCross::runBar(){
    //execute orders if the exist the previous day here
    if(check){
        broker.createOrder(nextOrder);
        check = false;
    }
    
    //add value to both sums and chekc if queues are filled(connectBar will have the most recent Bar)
    double currPrice = connectBar.close;
    
    fastSum += currPrice;
    slowSum += currPrice;

    fastWindow.push(currPrice);
    slowWindow.push(currPrice);
    
    //check if the queues are filled up and pop if necessary
    if(fastWindow.size() > fastLength){
        fastSum -= fastWindow.front();
        fastWindow.pop();
    }
    if(slowWindow.size() > slowLength){
        slowSum -= slowWindow.front();
        slowWindow.pop();
    }

    //if both windows are filled then execute sma logic
    if(fastWindow.size() == fastLength && slowWindow.size() == slowLength){
        //calculuate averages
        double fastAverage = fastSum/fastLength;
        double slowAverage = slowSum/slowLength;

        //check if fast is larger than slow => golden cross
        if(fastAverage > slowAverage){
            double currBalance = user.checkBalance();
            //calculate 20% of currBalance worth in shares(floored in order to have int shares)
            long numShares = std::floor((currBalance*0.2)/currPrice);
            //create Order struct
            nextOrder = {ticker, "market", 0, numShares, -1.0};
            check = true;
        }
        else if(fastAverage < slowAverage){
            long numShares = user.checkPosition(ticker);
            nextOrder = {ticker, "market", 1, numShares, -1.0};
            check = true;
        }
    }
    
}