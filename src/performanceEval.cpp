#include "../include/performanceEval.hpp"
#include <algorithm>

Metrics::Metrics(Account& a, std::unordered_map<long int, Trade>& tH, std::vector<std::string> all) : user(a), tradeHistory(tH), allTickers(all){
    
}

double Metrics::totalReturn(double initial, std::unordered_map<std::string, double> currPrices){
    //totalReturn = (final equity - intial equity)/initial equity * 100
    double finalEquity;
    double initialEquity = initial;
    finalEquity = user.accountValue(currPrices);
    std::cout << "Initial Equity: " << initialEquity << std::endl;
    std::cout << "Final Equity: " << finalEquity << std::endl;
    return ((finalEquity-initialEquity)/initialEquity)*100.0;
    
}

//similar to totalReturn but accounts for the time elapsed to obtain a average annual growth rate
//good value beats S&P 500
double Metrics::cagr(double initial, std::unordered_map<std::string, double> currPrices, int years){
    //cagr = ((final/initial)^(1/Y)-1)*100
    //converts to percentage
    double finalEquity;
    double initialEquity = initial;
    finalEquity = user.accountValue(currPrices);
    double base = finalEquity/initialEquity;
    double exp = 1.0/years;
    return (std::pow(base, exp) - 1.0)*100.0;
}

//calculates the drawdown from the highest value reached to the current value of all assets
double Metrics::drawDown(std::unordered_map<std::string, double> currPrices){
    double current = user.accountValue(currPrices);
    if(current > peakValue){
        peakValue = current;
    }

    //calculate drawdown
    double dd = (current - peakValue)/peakValue * 100.0;
    if(dd < minDrawdownSeen){
        minDrawdownSeen = dd;
    }
    return dd;

}

//overloaded if the value is already calculated
double Metrics::drawDown(double value){

    if(value > peakValue){
        peakValue = value;
    }

    //calculate drawdown
    double dd = (value - peakValue)/peakValue * 100.0;
    if(dd < minDrawdownSeen){
        minDrawdownSeen = dd;
    }
    return dd;

}

double Metrics::maxDrawdown() const{
    return minDrawdownSeen;
}

std::vector<double> Metrics::returnsFromEquityCurve(const std::vector<double>& equityCurve){
    std::vector<double> returns;
    if(equityCurve.size() < 2){
        return returns;
    }
    returns.reserve(equityCurve.size()-1);
    for(size_t i = 0; i+1 < equityCurve.size(); i++){
        returns.push_back(equityCurve[i+1]/equityCurve[i] - 1.0);
    }
    return returns;
}

double Metrics::sharpeRatio(const std::vector<double>& returns, double riskFreeRate, double periodsPerYear) const{
    size_t n = returns.size();
    if(n < 2){
        return 0.0;
    }

    double mean = 0.0;
    for(double r : returns){
        mean += r;
    }
    mean /= n;

    double sumSquaredDiff = 0.0;
    for(double r : returns){
        sumSquaredDiff += std::pow(r-mean, 2);
    }
    double stdDev = std::sqrt(sumSquaredDiff/(n-1));

    // Exact equality against 0.0 is unreliable here: floating-point returns
    // that are conceptually identical (e.g. three "0.05" values) can still
    // produce a tiny nonzero stdDev due to rounding, which would otherwise
    // blow this ratio up to a huge, meaningless number instead of the
    // documented 0.0 sentinel.
    if(stdDev < 1e-9){
        return 0.0;
    }

    return ((mean-riskFreeRate)/stdDev) * std::sqrt(periodsPerYear);
}

double Metrics::sortinoRatio(const std::vector<double>& returns, double riskFreeRate, double periodsPerYear) const{
    size_t n = returns.size();
    if(n < 2){
        return 0.0;
    }

    double mean = 0.0;
    for(double r : returns){
        mean += r;
    }
    mean /= n;

    double sumSquaredDownside = 0.0;
    for(double r : returns){
        double downside = std::min(0.0, r-riskFreeRate);
        sumSquaredDownside += downside*downside;
    }
    double downsideDev = std::sqrt(sumSquaredDownside/n);

    // Same floating-point-tolerance reasoning as sharpeRatio's stdDev check.
    if(downsideDev < 1e-9){
        return 0.0;
    }

    return ((mean-riskFreeRate)/downsideDev) * std::sqrt(periodsPerYear);
}

double Metrics::benchmarkReturn(double startPrice, double endPrice) const{
    return (endPrice-startPrice)/startPrice * 100.0;
}