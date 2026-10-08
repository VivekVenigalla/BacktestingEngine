#pragma once
#include "../strategy.hpp"

//NOTES ABOUT STRATEGY
//This strategy is built on the RSI(Relative Strength Index), a momentum
//oscillator that squeezes recent price movement into a single number between
//0 and 100:
//  near 0   = price has been falling almost every bar(oversold)
//  near 100 = price has been rising almost every bar(overbought)
//  near 50  = ups and downs have been about balanced
//The mean-reversion idea is that a stretch that extreme tends to snap back:
//buy when RSI drops below an oversold level(classically 30), and sell when it
//climbs above an overbought level(classically 70).
//
//How it differs from the other strategies so far: smaCross/bollBand/donChannel
//all work from plain rolling windows of prices(a sum, a max/min). RSI instead
//needs a SMOOTHED running average that remembers all of history, with older
//bars fading out gradually rather than dropping off a cliff - see
//RsiCalculator below.
//
//STATUS: this file currently holds the indicator and the zone classification
//only. runBar() computes the RSI every bar but places NO orders yet - the
//entry/exit logic is the next step.

//the pure RSI math, kept separate from Strategy so it can be checked on its
//own against known reference values(and reused by later strategies) without
//needing a Broker/Account/Bar feed around it
//
//Wilder's method, for a period of N(classically 14):
//  1. each bar, take the change from the previous close: a gain(if up) or a
//     loss(if down) - the other one is 0, and the loss is stored as a positive
//     number
//  2. after the first N changes, start with the plain AVERAGE gain and average
//     loss over those N changes
//  3. from then on, update with Wilder's smoothing:
//        avgGain = (previousAvgGain * (N-1) + todaysGain) / N
//        avgLoss = (previousAvgLoss * (N-1) + todaysLoss) / N
//     i.e. each new bar counts for 1/N and everything before it for (N-1)/N
//     - an exponential moving average that never fully forgets old bars
//  4. RS = avgGain / avgLoss, and RSI = 100 - 100 / (1 + RS)
//
//so the first RSI needs N+1 prices(N changes) - before that, ready() is false
class RsiCalculator{
    public:
        //throws std::invalid_argument if period < 1, since a config mistake
        //like that is worth failing loudly on rather than quietly clamping
        explicit RsiCalculator(int period);

        //feed the next close in, oldest to newest
        void update(double price);

        //true once N changes have been seen(N+1 prices) - until then there is
        //not enough history for a real RSI
        bool ready() const { return isReady; }

        //the current RSI, 0-100. returns 50(neutral) while !ready() so that
        //anything classifying it can't mistake "no data yet" for a signal -
        //check ready() when the difference matters
        double value() const { return rsi; }

        int period() const { return periodLength; }

    private:
        int periodLength;
        bool hasPrevClose = false;
        double prevClose = 0.0;
        //how many price changes have been absorbed so far
        int changesSeen = 0;
        //during the first N changes these hold running TOTALS; once N is
        //reached they're converted to averages and then smoothed from there
        double avgGain = 0.0;
        double avgLoss = 0.0;
        bool isReady = false;
        double rsi = 50.0;

        void recomputeRsi();
};

class rsiReversion : public Strategy{
    public:
        rsiReversion(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::string symbol);
        rsiReversion(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::string symbol, int period);
        rsiReversion(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::string symbol, int period, double oversold, double overbought);
        rsiReversion(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::string symbol, int period, double oversold, double overbought, double posSizePct);
        void runBar();
        void init();
        virtual ~rsiReversion() = default;

        //where the latest RSI sits relative to the two thresholds:
        //  -1 = oversold(RSI below the oversold level - the "buy" side)
        //   0 = neutral(between the levels, or not enough history yet)
        //  +1 = overbought(RSI above the overbought level - the "sell" side)
        //exactly on a threshold counts as neutral - only strictly beyond it
        //is a signal
        int zone() const;

        double currentRsi() const { return rsiCalc.value(); }
        bool rsiReady() const { return rsiCalc.ready(); }
        double oversoldLevel() const { return oversold; }
        double overboughtLevel() const { return overbought; }

    private:
        double oversold = 30.0;
        double overbought = 70.0;
        RsiCalculator rsiCalc;
};
