#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "performanceEval.hpp"

TEST_CASE("totalReturn computes percentage return from initial to current equity", "[metrics]") {
    Account acct(10000.0);
    acct.modifyBalance(1500.0); //equity now 11500, no open positions
    std::unordered_map<long int, Trade> history;
    //"{}" as the third constructor argument passes an empty
    //std::vector<std::string> - the allTickers list isn't needed by any of
    //the methods exercised in this file, so it's left empty everywhere below
    Metrics calc(acct, history, {});
    std::unordered_map<std::string, double> prices; //empty: accountValue is cash-only here

    //(11500 - 10000) / 10000 * 100 = 15.0
    REQUIRE(calc.totalReturn(10000.0, prices) == Catch::Approx(15.0));
}

TEST_CASE("totalReturn accounts for open position value via currPrices", "[metrics]") {
    Account acct(10000.0);
    acct.buyNewPosition("AAPL", 10, 100.0); //balance: 10000 - 1000 = 9000
    std::unordered_map<long int, Trade> history;
    Metrics calc(acct, history, {"AAPL"});
    std::unordered_map<std::string, double> prices{{"AAPL", 150.0}};

    //equity = 9000 (cash) + 10*150 (AAPL) = 10500 -> (10500-10000)/10000*100 = 5.0
    REQUIRE(calc.totalReturn(10000.0, prices) == Catch::Approx(5.0));
}

TEST_CASE("cagr computes annualized growth rate over a multi-year period", "[metrics]") {
    Account acct(10000.0);
    acct.modifyBalance(2100.0); //equity now 12100
    std::unordered_map<long int, Trade> history;
    Metrics calc(acct, history, {});
    std::unordered_map<std::string, double> prices;

    //base = 12100/10000 = 1.21; over 2 years, (1.21^0.5 - 1)*100 = (1.1-1)*100 = 10.0
    REQUIRE(calc.cagr(10000.0, prices, 2) == Catch::Approx(10.0));
}

TEST_CASE("cagr over a single year matches the simple total return", "[metrics]") {
    Account acct(10000.0);
    acct.modifyBalance(500.0); //equity now 10500
    std::unordered_map<long int, Trade> history;
    Metrics calc(acct, history, {});
    std::unordered_map<std::string, double> prices;

    //base = 10500/10000 = 1.05; over 1 year, (1.05^1 - 1)*100 = 5.0
    REQUIRE(calc.cagr(10000.0, prices, 1) == Catch::Approx(5.0));
}

TEST_CASE("drawDown(value) is zero at a new peak and negative below the peak", "[metrics]") {
    Account acct(10000.0);
    std::unordered_map<long int, Trade> history;
    Metrics calc(acct, history, {});

    REQUIRE(calc.drawDown(10000.0) == Catch::Approx(0.0)); //first call establishes the peak
    REQUIRE(calc.drawDown(12000.0) == Catch::Approx(0.0)); //new peak -> still zero drawdown

    //fallen to 10800, 10% below the 12000 peak: (10800-12000)/12000*100 = -10.0
    REQUIRE(calc.drawDown(10800.0) == Catch::Approx(-10.0));

    //partial recovery to 11000, still below the 12000 peak: (11000-12000)/12000*100
    REQUIRE(calc.drawDown(11000.0) == Catch::Approx(-8.3333).margin(0.001));
}

TEST_CASE("drawDown(currPrices) values the account before comparing to the peak", "[metrics]") {
    Account acct(10000.0);
    acct.buyNewPosition("AAPL", 10, 100.0); //balance: 10000 - 1000 = 9000
    std::unordered_map<long int, Trade> history;
    Metrics calc(acct, history, {"AAPL"});

    std::unordered_map<std::string, double> pricesAtPeak{{"AAPL", 150.0}}; //equity = 9000+1500 = 10500
    REQUIRE(calc.drawDown(pricesAtPeak) == Catch::Approx(0.0));

    std::unordered_map<std::string, double> pricesDown{{"AAPL", 120.0}}; //equity = 9000+1200 = 10200
    //(10200-10500)/10500*100
    REQUIRE(calc.drawDown(pricesDown) == Catch::Approx(-2.857142857).margin(0.0001));
}

TEST_CASE("maxDrawdown reports the worst drawdown seen, not just the current one", "[metrics]") {
    Account acct(10000.0);
    std::unordered_map<long int, Trade> history;
    Metrics calc(acct, history, {});

    calc.drawDown(10000.0); //establishes the first peak
    calc.drawDown(12000.0); //new peak, drawdown still 0
    calc.drawDown(10800.0); //10% below the 12000 peak
    REQUIRE(calc.maxDrawdown() == Catch::Approx(-10.0));

    calc.drawDown(11000.0); //partial recovery -- doesn't erase the worse -10% already seen
    REQUIRE(calc.maxDrawdown() == Catch::Approx(-10.0));

    calc.drawDown(13000.0); //a new peak again
    REQUIRE(calc.maxDrawdown() == Catch::Approx(-10.0)); //still the worst ever seen
}

TEST_CASE("returnsFromEquityCurve converts equity levels into period returns", "[metrics]") {
    //10500/10000-1 = 0.05 exactly; 9975/10500-1 = -0.05 exactly
    //returnsFromEquityCurve is a static method(see performanceEval.hpp) so
    //it's called as Metrics::returnsFromEquityCurve(...) - through the
    //class name, not through a Metrics object like calc.drawDown(...) above
    std::vector<double> returns = Metrics::returnsFromEquityCurve({10000.0, 10500.0, 9975.0});

    REQUIRE(returns.size() == 2);
    REQUIRE(returns[0] == Catch::Approx(0.05));
    REQUIRE(returns[1] == Catch::Approx(-0.05));
}

TEST_CASE("returnsFromEquityCurve returns empty for fewer than two points", "[metrics]") {
    //.empty() reports whether a vector has zero elements - cheaper and
    //clearer than checking ".size() == 0"
    REQUIRE(Metrics::returnsFromEquityCurve({}).empty());
    REQUIRE(Metrics::returnsFromEquityCurve({10000.0}).empty());
}

TEST_CASE("sharpeRatio computes an annualized risk-adjusted return", "[metrics]") {
    Account acct(10000.0);
    std::unordered_map<long int, Trade> history;
    Metrics calc(acct, history, {});

    std::vector<double> returns{0.02, 0.04, 0.06};
    //mean=0.04, sample stddev=0.02 -> (0.04/0.02)*sqrt(1) = 2.0
    REQUIRE(calc.sharpeRatio(returns, 0.0, 1.0) == Catch::Approx(2.0));
    //same returns annualized over 4 periods/year -> 2.0*sqrt(4) = 4.0
    REQUIRE(calc.sharpeRatio(returns, 0.0, 4.0) == Catch::Approx(4.0));
}

TEST_CASE("sharpeRatio is clamped to zero when returns have no variance", "[metrics]") {
    Account acct(10000.0);
    std::unordered_map<long int, Trade> history;
    Metrics calc(acct, history, {});

    std::vector<double> flatReturns{0.05, 0.05, 0.05};
    REQUIRE(calc.sharpeRatio(flatReturns, 0.0, 1.0) == Catch::Approx(0.0));
}

TEST_CASE("sortinoRatio computes an annualized downside-risk-adjusted return", "[metrics]") {
    Account acct(10000.0);
    std::unordered_map<long int, Trade> history;
    Metrics calc(acct, history, {});

    std::vector<double> returns{0.05, -0.02, 0.03, -0.01};
    //mean=0.0125, downside deviation=sqrt(1.25)/... closed form works out to sqrt(1.25)
    REQUIRE(calc.sortinoRatio(returns, 0.0, 1.0) == Catch::Approx(1.118033988749895));
}

TEST_CASE("sortinoRatio is clamped to zero when there are no losing periods", "[metrics]") {
    Account acct(10000.0);
    std::unordered_map<long int, Trade> history;
    Metrics calc(acct, history, {});

    std::vector<double> allPositive{0.02, 0.04, 0.06};
    REQUIRE(calc.sortinoRatio(allPositive, 0.0, 1.0) == Catch::Approx(0.0));
}

TEST_CASE("benchmarkReturn computes a naive buy-and-hold percentage return", "[metrics]") {
    Account acct(10000.0);
    std::unordered_map<long int, Trade> history;
    Metrics calc(acct, history, {});

    REQUIRE(calc.benchmarkReturn(100.0, 120.0) == Catch::Approx(20.0));
}

TEST_CASE("winRate and profitFactor summarize closed (filled sell) trades", "[metrics]") {
    Account acct(10000.0);

    //Four closed trades: two winners (+100, +200), two losers (-50, -25).
    //semicolons let you put several separate statements on one line -
    //"Trade win1; win1.side = 1; ..." is exactly the same as writing each
    //statement on its own line, just laid out compactly here since each
    //Trade only needs a couple of fields set for this test
    Trade win1; win1.side = 1; win1.filled = true; win1.realizedPnL = 100.0;
    Trade loss1; loss1.side = 1; loss1.filled = true; loss1.realizedPnL = -50.0;
    Trade win2; win2.side = 1; win2.filled = true; win2.realizedPnL = 200.0;
    Trade loss2; loss2.side = 1; loss2.filled = true; loss2.realizedPnL = -25.0;
    //A buy (side==0) and an unfilled sell -- neither counts as "closed".
    Trade buy; buy.side = 0; buy.filled = true; buy.realizedPnL = 0.0;
    Trade unfilledSell; unfilledSell.side = 1; unfilledSell.filled = false; unfilledSell.realizedPnL = 0.0;

    std::unordered_map<long int, Trade> history{
        {1, win1}, {2, loss1}, {3, win2}, {4, loss2}, {5, buy}, {6, unfilledSell}
    };
    Metrics calc(acct, history, {});

    //2 of 4 closed trades won -> 50%
    REQUIRE(calc.winRate() == Catch::Approx(50.0));
    //grossProfit=300, grossLoss=75 -> 300/75 = 4.0
    REQUIRE(calc.profitFactor() == Catch::Approx(4.0));
}

TEST_CASE("winRate and profitFactor are well-defined with no closed trades", "[metrics]") {
    Account acct(10000.0);
    std::unordered_map<long int, Trade> history; //empty
    Metrics calc(acct, history, {});

    REQUIRE(calc.winRate() == Catch::Approx(0.0));
    REQUIRE(calc.profitFactor() == Catch::Approx(-1.0)); //sentinel: undefined
}

TEST_CASE("profitFactor is a sentinel when there are no losing trades to divide by", "[metrics]") {
    Account acct(10000.0);
    Trade win1; win1.side = 1; win1.filled = true; win1.realizedPnL = 100.0;
    Trade win2; win2.side = 1; win2.filled = true; win2.realizedPnL = 50.0;
    std::unordered_map<long int, Trade> history{{1, win1}, {2, win2}};
    Metrics calc(acct, history, {});

    REQUIRE(calc.winRate() == Catch::Approx(100.0));
    REQUIRE(calc.profitFactor() == Catch::Approx(-1.0)); //sentinel: no losses
}
