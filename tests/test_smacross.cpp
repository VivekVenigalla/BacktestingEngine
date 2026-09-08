#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "strategies/smaCross.hpp"

// smaCross::fastSum/slowSum accumulate rolling sums for the fast/slow moving
// averages, but are never initialized (no default member initializer, and
// neither constructor sets them) -- the same class of bug as bollBand's
// uninitialized `temp` found earlier. These tests drive a handful of bars
// through runBar() and check the resulting order against a hand-computed
// crossover, which only comes out right if the sums start at zero.

TEST_CASE("smaCross places a buy order on a golden cross (fast average above slow)", "[smacross]") {
    Account acct(10000.0);
    std::unordered_map<std::string, Bar> bars;
    std::unordered_map<long int, Trade> history;
    Broker broker(acct, bars); // default commission $1.00

    smaCross strat(broker, acct, bars, history, "AAPL", /*fast*/2, /*slow*/4);

    // Closes chosen so the last-2 average (fast) ends up above the last-4
    // average (slow) on the 4th bar, once both windows are exactly full:
    //   fastWindow ends as [100, 140] -> avg 120
    //   slowWindow ends as [100, 100, 100, 140] -> avg 110
    for (double close : {100.0, 100.0, 100.0, 140.0}) {
        Bar bar;
        bar.ticker = "AAPL";
        bar.date = "2024-01-01";
        bar.open = close;
        bar.high = close;
        bar.low = close;
        bar.close = close;
        bar.volume = 1000;
        bars["AAPL"] = bar;
        strat.runBar();
    }

    REQUIRE(broker.returnOrders().size() == 1);
    const Order& placed = broker.returnOrders().begin()->second;
    REQUIRE(placed.side == 0); // buy
    // floor(10000 * 0.2 / 140) = floor(14.2857) = 14
    REQUIRE(placed.quantity == 14);
}

TEST_CASE("smaCross places a sell order on a death cross (fast average below slow)", "[smacross]") {
    Account acct(10000.0);
    acct.buyNewPosition("AAPL", 50, 90.0); // shares to sell on the death cross
    std::unordered_map<std::string, Bar> bars;
    std::unordered_map<long int, Trade> history;
    Broker broker(acct, bars);

    smaCross strat(broker, acct, bars, history, "AAPL", /*fast*/2, /*slow*/4);

    // Mirror image of the golden-cross series:
    //   fastWindow ends as [140, 100] -> avg 120
    //   slowWindow ends as [140, 140, 140, 100] -> avg 130
    for (double close : {140.0, 140.0, 140.0, 100.0}) {
        Bar bar;
        bar.ticker = "AAPL";
        bar.date = "2024-01-01";
        bar.open = close;
        bar.high = close;
        bar.low = close;
        bar.close = close;
        bar.volume = 1000;
        bars["AAPL"] = bar;
        strat.runBar();
    }

    REQUIRE(broker.returnOrders().size() == 1);
    const Order& placed = broker.returnOrders().begin()->second;
    REQUIRE(placed.side == 1); // sell
    REQUIRE(placed.quantity == 50); // sells the entire held position
}
