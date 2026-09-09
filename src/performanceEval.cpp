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
    //std::pow(base, exp) raises base to the power of exp - this is the
    //"nth root" trick: raising to the power of (1/years) is the same thing
    //as taking the years-th root, which spreads total growth evenly across
    //however many years the backtest covered
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
    //peakValue only ever goes up, so dd is always <= 0 here - this line
    //keeps a running record of the most negative dd has ever been, which
    //is what maxDrawdown() below hands back later, even after the account
    //recovers and dd itself climbs back toward 0
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

//const at the end means this method promises not to change any of
//Metrics's own fields - it just hands back the minDrawdownSeen the
//drawDown() calls above have already been tracking
double Metrics::maxDrawdown() const{
    return minDrawdownSeen;
}

std::vector<double> Metrics::returnsFromEquityCurve(const std::vector<double>& equityCurve){
    std::vector<double> returns;
    if(equityCurve.size() < 2){
        return returns;
    }
    //.reserve(n) tells the vector "you're going to hold about n items", so
    //it can allocate that much space once upfront instead of repeatedly
    //resizing itself as push_back adds more elements below - purely a
    //performance hint, doesn't change what the vector contains
    returns.reserve(equityCurve.size()-1);
    for(size_t i = 0; i+1 < equityCurve.size(); i++){
        //(next equity)/(current equity) - 1 is the standard formula for a
        //"percent change" between two numbers, expressed as a fraction
        //rather than a whole percentage(0.05 here means a 5% gain)
        returns.push_back(equityCurve[i+1]/equityCurve[i] - 1.0);
    }
    return returns;
}

double Metrics::sharpeRatio(const std::vector<double>& returns, double riskFreeRate, double periodsPerYear) const{
    size_t n = returns.size();
    if(n < 2){
        return 0.0;
    }

    //"for(double r : returns)" is a range-based for loop(c++11) - it walks
    //every element of returns one at a time into r, without needing an
    //index or an iterator. this loop just sums everything up to compute
    //the average(mean) return
    double mean = 0.0;
    for(double r : returns){
        mean += r;
    }
    mean /= n;

    //standard deviation measures how spread out the returns are from their
    //average - see bollBand.cpp's standardDeviation for the same formula
    //explained in more depth
    double sumSquaredDiff = 0.0;
    for(double r : returns){
        sumSquaredDiff += std::pow(r-mean, 2);
    }
    double stdDev = std::sqrt(sumSquaredDiff/(n-1));

    //exact equality against 0.0 is unreliable here: floating-point returns
    //that are conceptually identical(e.g three "0.05" values) can still
    //produce a tiny nonzero stdDev due to rounding, which would otherwise
    //blow this ratio up to a huge, meaningless number instead of the
    //documented 0.0 sentinel
    if(stdDev < 1e-9){
        return 0.0;
    }

    //dividing average return by how spread out it is gives the raw sharpe
    //ratio, and multiplying by sqrt(periodsPerYear) annualizes it(scales a
    //per-bar number up to a "per year" one) so a daily and a monthly
    //strategy's ratios can be compared on the same footing
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

    //same idea as sharpe's standard deviation above, but std::min(0.0, ...)
    //clamps every WINNING period's contribution to exactly 0 before
    //squaring it - so only losing periods(where r-riskFreeRate is
    //negative) actually add anything to sumSquaredDownside. that's what
    //makes this "downside" deviation instead of plain standard deviation
    double sumSquaredDownside = 0.0;
    for(double r : returns){
        double downside = std::min(0.0, r-riskFreeRate);
        sumSquaredDownside += downside*downside;
    }
    double downsideDev = std::sqrt(sumSquaredDownside/n);

    //same floating-point-tolerance reasoning as sharpeRatio's stdDev check.
    if(downsideDev < 1e-9){
        return 0.0;
    }

    return ((mean-riskFreeRate)/downsideDev) * std::sqrt(periodsPerYear);
}

double Metrics::benchmarkReturn(double startPrice, double endPrice) const{
    return (endPrice-startPrice)/startPrice * 100.0;
}

//a "closed" trade is a filled sell -- the point at which a realizedPnL exists
double Metrics::winRate() const{
    int closedTrades = 0;
    int wins = 0;

    //"const auto& [id, trade]" unpacks each(key, value) pair straight out
    //of tradeHistory(an unordered_map<long int, Trade>) - id is the trade's
    //map key, trade is a read-only reference to the actual Trade
    for(const auto& [id, trade] : tradeHistory){
        if(trade.side == 1 && trade.filled){
            closedTrades++;
            if(trade.realizedPnL > 0.0){
                wins++;
            }
        }
    }

    if(closedTrades == 0){
        return 0.0;
    }

    //wins and closedTrades are both int - dividing two ints in c++ does
    //INTEGER division(3/4 would give 0, not 0.75), so static_cast<double>
    //converts wins to a double first, forcing the division to keep its
    //decimal part
    return (static_cast<double>(wins)/closedTrades) * 100.0;
}

double Metrics::profitFactor() const{
    double grossProfit = 0.0;
    double grossLoss = 0.0;

    for(const auto& [id, trade] : tradeHistory){
        if(trade.side == 1 && trade.filled){
            if(trade.realizedPnL > 0.0){
                grossProfit += trade.realizedPnL;
            }
            else if(trade.realizedPnL < 0.0){
                //realizedPnL is negative here, so negating it(-trade.
                //realizedPnL) turns it back into a positive "how much was
                //lost" amount to add to the running total
                grossLoss += -trade.realizedPnL;
            }
        }
    }

    if(grossLoss < 1e-9){
        return -1.0; //undefined: no closed trades, or no losing trades to divide by
    }

    return grossProfit/grossLoss;
}
