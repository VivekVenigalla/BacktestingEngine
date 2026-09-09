#include "../../include/strategies/smaCross.hpp"
#include <cmath>
//cmath includes the operations such as floor
#include <iostream>


smaCross::smaCross(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::string symbol) : Strategy(b, u, cBs, history, symbol){
}

smaCross::smaCross(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::string symbol, int fast, int slow) : Strategy(b, u, cBs, history, symbol), fastLength(fast), slowLength(slow){
    if (fastLength >= slowLength) {
        std::cerr << "WARNING: Fast SMA period should be less than Slow SMA period. Adjusting values...\n";
        fastLength = 50;
        slowLength = 200;
    }
}

//": smaCross(b, u, cBs, history, symbol, fast, slow)" here is constructor
//delegation(c++11) - instead of repeating the fast/slow validation logic
//above, this constructor just calls the other one first, then only adds
//the one extra line(positionSizePct = posSizePct) it actually needs
smaCross::smaCross(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::string symbol, int fast, int slow, double posSizePct) : smaCross(b, u, cBs, history, symbol, fast, slow){
    positionSizePct = posSizePct;
}

void smaCross::init(){
    std::cout << "Created a SMA Strategy" << std::endl;
}

void smaCross::runBar(){

    //if a bracket is waiting on a buy that has since filled, place it now
    //(entry price comes from the account's own AEP, since that's the real
    //fill price rather than a remembered/guessed one)
    if(bracketPending && user.positionQuantity(ticker) >= bracketQuantity){
        placeBracketOrders(user.positionAEP(ticker), bracketQuantity, stopLossPct, takeProfitPct);
        bracketPending = false;
        bracketQuantity = 0;
    }

    //add value to both sums and chekc if queues are filled(connectBar will have the most recent Bar)
    double currPrice = connectBars.begin()->second.close;
    std::cout<<currPrice << std::endl;
    fastSum += currPrice;
    slowSum += currPrice;

    fastWindow.push(currPrice);
    slowWindow.push(currPrice);

    //check if the queues are filled up and pop if necessary
    //this is the incremental-average trick: instead of re-adding up every
    //price in the window each bar(slow, gets slower as the window grows),
    //just add the new price and subtract the one falling out of the window
    //- the running sum stays correct in constant time per bar
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
            //size the buy using the shared, configurable position-sizing helper
            long numShares = sizeBuyOrder(currPrice);
            //create Order struct
            nextOrder = {ticker, "market", 0, numShares, -1.0};
            broker.createOrder(nextOrder);
            //the buy is only queued, not filled yet -- place the protective
            //bracket once it actually fills (see bracketPending's comment)
            bracketPending = true;
            bracketQuantity = numShares;
        }
        else if(fastAverage < slowAverage){
            long numShares = user.positionQuantity(ticker);
            nextOrder = {ticker, "market", 1, numShares, -1.0};
            broker.createOrder(nextOrder);
        }
    }

}
