#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "performanceEval.hpp"

TEST_CASE("totalReturn computes percentage return from initial to current equity", "[metrics]") {
    Account acct(10000.0);
    acct.modifyBalance(1500.0); // equity now 11500, no open positions
    std::unordered_map<long int, Trade> history;
    Metrics calc(acct, history, {});
    std::unordered_map<std::string, double> prices; // empty: accountValue is cash-only here

    // (11500 - 10000) / 10000 * 100 = 15.0
    REQUIRE(calc.totalReturn(10000.0, prices) == Catch::Approx(15.0));
}

TEST_CASE("totalReturn accounts for open position value via currPrices", "[metrics]") {
    Account acct(10000.0);
    acct.buyNewPosition("AAPL", 10, 100.0); // balance: 10000 - 1000 = 9000
    std::unordered_map<long int, Trade> history;
    Metrics calc(acct, history, {"AAPL"});
    std::unordered_map<std::string, double> prices{{"AAPL", 150.0}};

    // equity = 9000 (cash) + 10*150 (AAPL) = 10500 -> (10500-10000)/10000*100 = 5.0
    REQUIRE(calc.totalReturn(10000.0, prices) == Catch::Approx(5.0));
}

TEST_CASE("cagr computes annualized growth rate over a multi-year period", "[metrics]") {
    Account acct(10000.0);
    acct.modifyBalance(2100.0); // equity now 12100
    std::unordered_map<long int, Trade> history;
    Metrics calc(acct, history, {});
    std::unordered_map<std::string, double> prices;

    // base = 12100/10000 = 1.21; over 2 years, (1.21^0.5 - 1)*100 = (1.1-1)*100 = 10.0
    REQUIRE(calc.cagr(10000.0, prices, 2) == Catch::Approx(10.0));
}

TEST_CASE("cagr over a single year matches the simple total return", "[metrics]") {
    Account acct(10000.0);
    acct.modifyBalance(500.0); // equity now 10500
    std::unordered_map<long int, Trade> history;
    Metrics calc(acct, history, {});
    std::unordered_map<std::string, double> prices;

    // base = 10500/10000 = 1.05; over 1 year, (1.05^1 - 1)*100 = 5.0
    REQUIRE(calc.cagr(10000.0, prices, 1) == Catch::Approx(5.0));
}

TEST_CASE("drawDown(value) is zero at a new peak and negative below the peak", "[metrics]") {
    Account acct(10000.0);
    std::unordered_map<long int, Trade> history;
    Metrics calc(acct, history, {});

    REQUIRE(calc.drawDown(10000.0) == Catch::Approx(0.0)); // first call establishes the peak
    REQUIRE(calc.drawDown(12000.0) == Catch::Approx(0.0)); // new peak -> still zero drawdown

    // fallen to 10800, 10% below the 12000 peak: (10800-12000)/12000*100 = -10.0
    REQUIRE(calc.drawDown(10800.0) == Catch::Approx(-10.0));

    // partial recovery to 11000, still below the 12000 peak: (11000-12000)/12000*100
    REQUIRE(calc.drawDown(11000.0) == Catch::Approx(-8.3333).margin(0.001));
}

TEST_CASE("drawDown(currPrices) values the account before comparing to the peak", "[metrics]") {
    Account acct(10000.0);
    acct.buyNewPosition("AAPL", 10, 100.0); // balance: 10000 - 1000 = 9000
    std::unordered_map<long int, Trade> history;
    Metrics calc(acct, history, {"AAPL"});

    std::unordered_map<std::string, double> pricesAtPeak{{"AAPL", 150.0}}; // equity = 9000+1500 = 10500
    REQUIRE(calc.drawDown(pricesAtPeak) == Catch::Approx(0.0));

    std::unordered_map<std::string, double> pricesDown{{"AAPL", 120.0}}; // equity = 9000+1200 = 10200
    // (10200-10500)/10500*100
    REQUIRE(calc.drawDown(pricesDown) == Catch::Approx(-2.857142857).margin(0.0001));
}
