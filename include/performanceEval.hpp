#pragma once
#include "./account.hpp"
#include "./structures.hpp"
#include "./broker.hpp"
#include <unordered_map>
#include <cmath>
//this class allows tha main logic to evaluate the performance of the strategy


//provides static methods
class Metrics{
    public:
        Metrics(Account& a,std::unordered_map<long int, Trade>& tH, std::vector<std::string> all);
        //returns the total return from the initial balance to all position value
        double totalReturn(double initial, std::unordered_map<std::string, double> currPrices);
        double cagr(double initial, std::unordered_map<std::string, double> currPrices, int years);
        double drawDown(std::unordered_map<std::string, double> currPrices);
        double drawDown(double value);
        //worst (most negative) drawdown percentage seen across all drawDown() calls so far
        double maxDrawdown() const;

        //converts a raw equity curve (one value per bar) into per-period returns
        static std::vector<double> returnsFromEquityCurve(const std::vector<double>& equityCurve);

        //risk-adjusted return measures computed from a period-return series.
        //riskFreeRate is a per-period rate (same units as the returns); periodsPerYear
        //is an external input (like cagr's `years`) since the engine only knows "bars",
        //not calendar frequency -- 252 (daily) is a reasonable default/stand-in.
        double sharpeRatio(const std::vector<double>& returns, double riskFreeRate = 0.0, double periodsPerYear = 252.0) const;
        double sortinoRatio(const std::vector<double>& returns, double riskFreeRate = 0.0, double periodsPerYear = 252.0) const;

        //percentage return of a naive buy-and-hold over the same period, for comparison
        double benchmarkReturn(double startPrice, double endPrice) const;

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