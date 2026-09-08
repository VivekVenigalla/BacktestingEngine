#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
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

// --- Baseline coverage: checkOrder's validation rules (not bugs, but worth
// pinning down so a future change can't silently break them) ---

TEST_CASE("checkOrder rejects a buy order when balance is insufficient", "[broker]") {
    Account acct(5.0);
    std::unordered_map<std::string, Bar> bars;
    Broker broker(acct, bars);

    Order order;
    order.ticker = "AAPL";
    order.type = "market";
    order.side = 0;
    order.quantity = 100;
    order.checkPrice = -1;

    int id = broker.createOrder(order);

    REQUIRE(broker.returnOrders().count(id) == 0); // never accepted as pending
    REQUIRE(broker.returnHistory()[id].filled == false);
}

TEST_CASE("checkOrder rejects a sell order when shares are insufficient", "[broker]") {
    Account acct(100000.0); // plenty of cash, but zero AAPL shares
    std::unordered_map<std::string, Bar> bars;
    Broker broker(acct, bars);

    Order order;
    order.ticker = "AAPL";
    order.type = "market";
    order.side = 1;
    order.quantity = 10;
    order.checkPrice = -1;

    int id = broker.createOrder(order);

    REQUIRE(broker.returnOrders().count(id) == 0);
    REQUIRE(broker.returnHistory()[id].filled == false);
}

TEST_CASE("checkOrder rejects a zero-quantity order", "[broker]") {
    Account acct(100000.0);
    std::unordered_map<std::string, Bar> bars;
    Broker broker(acct, bars);

    Order order;
    order.ticker = "AAPL";
    order.type = "market";
    order.side = 0;
    order.quantity = 0;
    order.checkPrice = -1;

    int id = broker.createOrder(order);

    REQUIRE(broker.returnHistory()[id].filled == false);
}

// --- Baseline coverage: checkOrderLimitAndStop's trigger conditions, and
// processOrder's execPrice math (slippage + commission), exercised together
// through the public checkLoop() -- checkOrderLimitAndStop and processOrder
// are private, so a pending order + checkLoop() is how the public API
// exercises them. This also covers all three order types' execPrice,
// standing in as the baseline test for the stop-order execPrice question
// investigated during planning (a fallback assignment after the type
// if/else already covers it -- there was no bug to fix, but this guards
// against ever removing that fallback without noticing). ---

TEST_CASE("processOrder fills pending orders via checkLoop with correct execPrice for every order type", "[broker]") {
    Bar bar;
    bar.ticker = "AAPL";
    bar.date = "2024-01-01";
    bar.open = 100.0;
    bar.high = 105.0;
    bar.low = 95.0;
    bar.close = 102.0;
    bar.volume = 1000;

    const double commission = 1.0;
    const double slippage = 0.01; // 1%, chosen for round numbers below

    SECTION("market buy") {
        Account acct(100000.0);
        std::unordered_map<std::string, Bar> bars{{"AAPL", bar}};
        Broker broker(acct, bars, commission, slippage, "b");

        Order order{"AAPL", "market", 0, 10, -1};
        int id = broker.createOrder(order);
        broker.checkLoop();

        REQUIRE(broker.returnHistory()[id].filled == true);
        // open*(1+slip)+commission = 100*1.01+1 = 102.0
        REQUIRE(broker.returnHistory()[id].execPrice == Catch::Approx(102.0));
    }

    SECTION("market sell") {
        Account acct(100000.0);
        acct.buyNewPosition("AAPL", 50, 90.0); // shares to sell
        std::unordered_map<std::string, Bar> bars{{"AAPL", bar}};
        Broker broker(acct, bars, commission, slippage, "b");

        Order order{"AAPL", "market", 1, 10, -1};
        int id = broker.createOrder(order);
        broker.checkLoop();

        // open*(1-slip)-commission = 100*0.99-1 = 98.0
        REQUIRE(broker.returnHistory()[id].execPrice == Catch::Approx(98.0));
    }

    SECTION("limit buy triggers when low <= checkPrice") {
        Account acct(100000.0);
        std::unordered_map<std::string, Bar> bars{{"AAPL", bar}};
        Broker broker(acct, bars, commission, slippage, "b");

        Order order{"AAPL", "limit", 0, 10, 98.0}; // low(95) <= 98 -> triggers
        int id = broker.createOrder(order);
        broker.checkLoop();

        REQUIRE(broker.returnHistory()[id].filled == true);
        // min(checkPrice, open)*(1+slip)+commission = min(98,100)*1.01+1 = 99.98
        REQUIRE(broker.returnHistory()[id].execPrice == Catch::Approx(99.98));
    }

    SECTION("limit sell triggers when high >= checkPrice") {
        Account acct(100000.0);
        acct.buyNewPosition("AAPL", 50, 90.0);
        std::unordered_map<std::string, Bar> bars{{"AAPL", bar}};
        Broker broker(acct, bars, commission, slippage, "b");

        Order order{"AAPL", "limit", 1, 10, 102.0}; // high(105) >= 102 -> triggers
        int id = broker.createOrder(order);
        broker.checkLoop();

        // max(checkPrice, open)*(1-slip)-commission = max(102,100)*0.99-1 = 99.98
        REQUIRE(broker.returnHistory()[id].execPrice == Catch::Approx(99.98));
    }

    SECTION("stop buy triggers when high >= checkPrice") {
        Account acct(100000.0);
        std::unordered_map<std::string, Bar> bars{{"AAPL", bar}};
        Broker broker(acct, bars, commission, slippage, "b");

        Order order{"AAPL", "stop", 0, 10, 104.0}; // high(105) >= 104 -> triggers
        int id = broker.createOrder(order);
        broker.checkLoop();

        REQUIRE(broker.returnHistory()[id].filled == true);
        // max(checkPrice, open)*(1+slip)+commission = max(104,100)*1.01+1 = 106.04
        REQUIRE(broker.returnHistory()[id].execPrice == Catch::Approx(106.04));
    }

    SECTION("stop sell triggers when low <= checkPrice") {
        Account acct(100000.0);
        acct.buyNewPosition("AAPL", 50, 90.0);
        std::unordered_map<std::string, Bar> bars{{"AAPL", bar}};
        Broker broker(acct, bars, commission, slippage, "b");

        Order order{"AAPL", "stop", 1, 10, 96.0}; // low(95) <= 96 -> triggers
        int id = broker.createOrder(order);
        broker.checkLoop();

        // min(checkPrice, open)*(1-slip)-commission = min(96,100)*0.99-1 = 94.04
        REQUIRE(broker.returnHistory()[id].execPrice == Catch::Approx(94.04));
    }
}

TEST_CASE("a limit order that hasn't reached its trigger price stays pending", "[broker]") {
    Bar bar;
    bar.ticker = "AAPL";
    bar.date = "2024-01-01";
    bar.open = 100.0;
    bar.high = 105.0;
    bar.low = 95.0;
    bar.close = 102.0;
    bar.volume = 1000;

    Account acct(100000.0);
    std::unordered_map<std::string, Bar> bars{{"AAPL", bar}};
    Broker broker(acct, bars);

    Order order{"AAPL", "limit", 0, 10, 50.0}; // low(95) > 50 -> never triggers on this bar
    int id = broker.createOrder(order);
    broker.checkLoop();

    REQUIRE(broker.returnOrders().count(id) == 1); // still pending
    REQUIRE(broker.returnHistory().count(id) == 0); // no trade recorded yet
}
