#include <catch2/catch_test_macros.hpp>
#include "broker.hpp"

// Trade::side is meant to hold a 0 (buy) / 1 (sell) flag, mirroring the
// Order it came from. Broker::createOrder's failure branch and
// Broker::deleteOrder both accidentally wrote the order's *quantity* into
// that field instead. These two tests each use an order whose quantity is
// deliberately different from its side, so a wrong assignment is obvious.

TEST_CASE("createOrder logs the order's actual side (not its quantity) when the order fails validation", "[broker]") {
    Account acct(10.0); // too little balance to afford the order below
    std::unordered_map<std::string, Bar> bars; // empty: Broker will look up "AAPL" and get a zero-valued default Bar
    Broker broker(acct, bars);

    Order order;
    order.ticker = "AAPL";
    order.type = "market";
    order.side = 0; // buy
    order.quantity = 500; // costs far more than the $10 balance -> checkOrder fails
    order.checkPrice = -1;

    int id = broker.createOrder(order);
    const Trade& logged = broker.returnHistory()[id];

    REQUIRE(logged.filled == false);
    REQUIRE(logged.side == 0);
}

TEST_CASE("deleteOrder logs the order's actual side (not its quantity) when cancelling a pending order", "[broker]") {
    Account acct(100000.0); // plenty of balance so the order below is accepted as pending
    std::unordered_map<std::string, Bar> bars;
    Broker broker(acct, bars);

    Order order;
    order.ticker = "AAPL";
    order.type = "market";
    order.side = 0; // buy
    order.quantity = 42; // deliberately different from `side` so a mix-up is obvious
    order.checkPrice = -1;

    int id = broker.createOrder(order); // accepted -> lands in the pending orders map
    broker.deleteOrder(id, "test cancellation");
    const Trade& logged = broker.returnHistory()[id];

    // quantity==42 confirms the cancellation record actually landed under
    // `id` (a missing/misfiled record would auto-vivify as a zero-valued
    // Trade here instead, with quantity==0).
    REQUIRE(logged.quantity == 42);
    REQUIRE(logged.side == 0);
}
