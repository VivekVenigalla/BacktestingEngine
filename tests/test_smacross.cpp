#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <algorithm>
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

TEST_CASE("smaCross places a protective bracket once its buy order actually fills", "[smacross]") {
    Account acct(10000.0);
    std::unordered_map<std::string, Bar> bars;
    std::unordered_map<long int, Trade> history;
    Broker broker(acct, bars, /*commission*/0.0, /*slippage*/0.0, "b");

    smaCross strat(broker, acct, bars, history, "AAPL", /*fast*/2, /*slow*/4);

    // checkLoop() is called before runBar() each bar here, mirroring
    // SimulationRunner::step()'s real ordering -- a market buy is only
    // queued by createOrder(), so it fills on the NEXT bar's checkLoop(),
    // not the bar it was requested on.
    auto feedBar = [&](double open, double close) {
        Bar bar;
        bar.ticker = "AAPL";
        bar.date = "2024-01-01";
        bar.open = open;
        bar.high = std::max(open, close);
        bar.low = std::min(open, close);
        bar.close = close;
        bar.volume = 1000;
        bars["AAPL"] = bar;
        broker.checkLoop();
        strat.runBar();
    };

    // Same golden-cross series as the test above: fires on the 4th bar.
    feedBar(100.0, 100.0);
    feedBar(100.0, 100.0);
    feedBar(100.0, 100.0);
    feedBar(140.0, 140.0); // golden cross -> buy queued, not yet filled

    REQUIRE(broker.returnOrders().size() == 1); // just the pending buy

    // 5th bar: checkLoop() (called first, inside feedBar) fills the queued
    // buy at this bar's open (140.0); runBar() then sees the position exists
    // and places the bracket. Close is chosen (60.0) so the fast/slow
    // averages land exactly equal afterward -- otherwise a strategy this
    // simple would immediately re-enter (or exit) on the same bar and add a
    // 3rd, unrelated order, muddying this test's specific assertion.
    feedBar(140.0, 60.0);

    REQUIRE(broker.returnOrders().size() == 2); // stop-sell + limit-sell
    bool foundStop = false, foundLimit = false;
    for (const auto& [id, order] : broker.returnOrders()) {
        REQUIRE(order.side == 1);
        REQUIRE(order.quantity == 14);
        if (order.type == "stop") {
            foundStop = true;
            REQUIRE(order.checkPrice == Catch::Approx(140.0 * 0.95)); // AEP=140.0 (the fill price), 5% below
        } else if (order.type == "limit") {
            foundLimit = true;
            REQUIRE(order.checkPrice == Catch::Approx(140.0 * 1.10)); // 10% above
        }
    }
    REQUIRE(foundStop);
    REQUIRE(foundLimit);
}
