#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "strategies/donChannel.hpp"

// donChannel checks the breakout condition using the PREVIOUS windowSize
// bars only -- the current bar's own close is pushed onto the window after
// the order-creation logic runs, so it doesn't participate in that call's
// min/max. With windowSize=4, the breakout logic first fires on the 5th
// call, using the first 4 closes.
TEST_CASE("donChannel places stop orders just outside the prior window's high/low", "[donchannel]") {
    Account acct(10000.0);
    acct.buyNewPosition("AAPL", 50, 90.0); // shares available for the sell-stop
    std::unordered_map<std::string, Bar> bars;
    std::unordered_map<long int, Trade> history;
    Broker broker(acct, bars);

    donChannel strat(broker, acct, bars, history, "AAPL", /*window*/4);

    auto feedBar = [&](double close) {
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
    };

    // Fill the 4-bar window: high 105, low 95.
    feedBar(100.0);
    feedBar(105.0);
    feedBar(95.0);
    feedBar(102.0);
    // 5th call: window is full, breakout logic fires using [100,105,95,102].
    feedBar(110.0);

    REQUIRE(broker.returnOrders().size() == 2);

    bool foundBuyStop = false;
    bool foundSellStop = false;
    for (const auto& [id, order] : broker.returnOrders()) {
        if (order.side == 0) {
            foundBuyStop = true;
            REQUIRE(order.type == "stop");
            REQUIRE(order.checkPrice == Catch::Approx(105.01)); // 1 cent above the window high
            REQUIRE(order.quantity == 18); // floor(10000*0.2/110)
        } else {
            foundSellStop = true;
            REQUIRE(order.type == "stop");
            REQUIRE(order.checkPrice == Catch::Approx(94.99)); // 1 cent below the window low
            REQUIRE(order.quantity == 10); // floor(50*0.2)
        }
    }
    REQUIRE(foundBuyStop);
    REQUIRE(foundSellStop);
}
