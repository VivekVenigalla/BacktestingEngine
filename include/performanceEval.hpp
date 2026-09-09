#pragma once
#include "./account.hpp"
#include "./structures.hpp"
#include "./broker.hpp"
#include <unordered_map>
#include <cmath>
//this class allows tha main logic to evaluate the performance of the strategy


//Metrics turns a finished(or in-progress) simulation into the numbers a
//trader actually cares about - not just "did it make money" but "how much
//risk did it take to get there". it reads from Account/tradeHistory but
//never modifies them - this class only observes and reports
//provides static methods
class Metrics{
    public:
        Metrics(Account& a,std::unordered_map<long int, Trade>& tH, std::vector<std::string> all);
        //returns the total return from the initial balance to all position value
        double totalReturn(double initial, std::unordered_map<std::string, double> currPrices);
        double cagr(double initial, std::unordered_map<std::string, double> currPrices, int years);
        double drawDown(std::unordered_map<std::string, double> currPrices);
        double drawDown(double value);
        //drawdown is how far equity has fallen from its highest point so
        //far, as a percentage - drawDown() above only reports the CURRENT
        //dip, which recovers back toward 0 once equity makes a new high.
        //maxDrawdown() instead remembers the single worst dip that ever
        //happened, even after the account has since recovered past it -
        //this is usually the number people mean by "max drawdown" when
        //judging how risky a strategy was to hold
        //worst (most negative) drawdown percentage seen across all drawDown() calls so far
        double maxDrawdown() const;

        //converts a raw equity curve (one value per bar) into per-period returns
        static std::vector<double> returnsFromEquityCurve(const std::vector<double>& equityCurve);

        //the sharpe ratio answers "how much return did this strategy earn
        //per unit of risk taken", where risk is measured as how much the
        //returns bounce around(their standard deviation) - two strategies
        //that make the same profit are NOT equally good if one got there
        //by swinging wildly and the other rose smoothly, and sharpe ratio
        //is the standard way to tell them apart. sortino ratio is the same
        //idea but only counts downside swings(losses) as "risk", since
        //most people don't actually mind volatility that comes from
        //winning periods, just from losing ones
        //risk-adjusted return measures computed from a period-return series.
        //riskFreeRate is a per-period rate (same units as the returns); periodsPerYear
        //is an external input (like cagr's `years`) since the engine only knows "bars",
        //not calendar frequency -- 252 (daily) is a reasonable default/stand-in.
        double sharpeRatio(const std::vector<double>& returns, double riskFreeRate = 0.0, double periodsPerYear = 252.0) const;
        double sortinoRatio(const std::vector<double>& returns, double riskFreeRate = 0.0, double periodsPerYear = 252.0) const;
        //"= 0.0" and "= 252.0" above are default arguments - calling
        //sharpeRatio(returns) with just one argument is allowed, and
        //riskFreeRate/periodsPerYear silently fill in with those defaults.
        //you only need to pass your own values if you want something
        //other than the default

        //percentage return of a naive buy-and-hold over the same period, for comparison
        double benchmarkReturn(double startPrice, double endPrice) const;

        //win rate is simply what fraction of closed trades made money.
        //profit factor compares TOTAL winnings to TOTAL losses(e.g a
        //profit factor of 2.0 means the strategy made $2 for every $1 it
        //lost) - a strategy can have a low win rate and still be
        //profitable overall if its average win is much bigger than its
        //average loss, which is what profit factor captures that win rate
        //alone can't
        //trade-level stats over tradeHistory's closed (filled sell) trades.
        //winRate is 0-100; profitFactor is grossProfit/grossLoss, with -1.0 as an
        //explicit "undefined" sentinel (no closed trades, or no losing trades to
        //divide by) so the value stays a normal, JSON-serializable number.
        double winRate() const;
        double profitFactor() const;

    private:
        Account& user;
        std::unordered_map<long int, Trade>& tradeHistory;
        std::vector<std::string> allTickers;
        double peakValue = 0.0;
        double minDrawdownSeen = 0.0;

};
