#include "../include/performanceEval.hpp"

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
    return (current - peakValue)/peakValue * 100.0;

}

//overloaded if the value is already calculated
double Metrics::drawDown(double value){
    
    if(value > peakValue){
        peakValue = value;
    }

    //calculate drawdown
    return (value - peakValue)/peakValue * 100.0;

}