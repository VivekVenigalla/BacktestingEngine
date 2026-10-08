#include "../../include/strategies/rsiReversion.hpp"
#include <iostream>
#include <stdexcept>

RsiCalculator::RsiCalculator(int period) : periodLength(period){
    if(period < 1){
        throw std::invalid_argument("RsiCalculator: period must be at least 1");
    }
}

void RsiCalculator::update(double price){
    //the very first price has nothing to compare against - it only sets the
    //baseline the first change is measured from
    if(!hasPrevClose){
        prevClose = price;
        hasPrevClose = true;
        return;
    }

    double change = price - prevClose;
    prevClose = price;

    //split the change into a gain and a loss - one of them is always 0, and
    //the loss is kept as a positive size so the math below stays simple
    double gain = change > 0.0 ? change : 0.0;
    double loss = change < 0.0 ? -change : 0.0;

    changesSeen++;

    if(changesSeen <= periodLength){
        //still collecting the first N changes: just add them up
        avgGain += gain;
        avgLoss += loss;

        if(changesSeen == periodLength){
            //we now have exactly N changes - turn the totals into plain
            //averages, which is Wilder's starting point, and the RSI becomes
            //real for the first time
            avgGain /= periodLength;
            avgLoss /= periodLength;
            isReady = true;
            recomputeRsi();
        }
    }
    else{
        //Wilder's smoothing: the old average keeps (N-1)/N of its weight and
        //today's gain/loss gets 1/N
        avgGain = (avgGain * (periodLength - 1) + gain) / periodLength;
        avgLoss = (avgLoss * (periodLength - 1) + loss) / periodLength;
        recomputeRsi();
    }
}

void RsiCalculator::recomputeRsi(){
    if(avgLoss == 0.0){
        //no losses at all in the smoothed history: RS would divide by zero.
        //if there are gains that's the maximum reading(100); if there's no
        //movement either, there is no momentum either way, so neutral(50)
        rsi = (avgGain == 0.0) ? 50.0 : 100.0;
        return;
    }
    double rs = avgGain / avgLoss;
    rsi = 100.0 - 100.0 / (1.0 + rs);
}

rsiReversion::rsiReversion(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::string symbol) : Strategy(b, u, cBs, history, symbol), rsiCalc(14){
}
rsiReversion::rsiReversion(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::string symbol, int period) : Strategy(b, u, cBs, history, symbol), rsiCalc(period){
}
rsiReversion::rsiReversion(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::string symbol, int period, double oversoldLevel, double overboughtLevel) : Strategy(b, u, cBs, history, symbol), oversold(oversoldLevel), overbought(overboughtLevel), rsiCalc(period){
}
//delegates to the constructor above(see smaCross.cpp for what constructor
//delegation means) instead of repeating it, then only sets the one extra field
rsiReversion::rsiReversion(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::string symbol, int period, double oversoldLevel, double overboughtLevel, double posSizePct) : rsiReversion(b, u, cBs, history, symbol, period, oversoldLevel, overboughtLevel){
    positionSizePct = posSizePct;
}

void rsiReversion::init(){
    std::cout << "Created an RSI(Mean Reversion) Strategy" << std::endl;
}

void rsiReversion::runBar(){
    double currPrice = connectBars.begin()->second.close;

    //keep the RSI current every bar. NO orders are placed yet - this
    //skeleton only computes and exposes the signal(see zone())
    rsiCalc.update(currPrice);
}

int rsiReversion::zone() const{
    if(!rsiCalc.ready()){
        return 0;
    }
    double rsi = rsiCalc.value();
    if(rsi < oversold){
        return -1;
    }
    if(rsi > overbought){
        return 1;
    }
    return 0;
}
