#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "broker.hpp"

//Trade::side is meant to hold a 0 (buy) / 1 (sell) flag, mirroring the
//Order it came from. Broker::createOrder's failure branch and
//Broker::deleteOrder both accidentally wrote the order's *quantity* into
//that field instead. These two tests each use an order whose quantity is
//deliberately different from its side, so a wrong assignment is obvious.

TEST_CASE("createOrder logs the order's actual side (not its quantity) when the order fails validation", "[broker]") {
    //commission is now a flat fee added once (not multiplied by quantity),
    //so with a $0-priced bar (see the empty `bars` map below) the balance
    //has to sit below the flat commisionFee itself for this order to
    //genuinely fail -- $10 would incorrectly pass now that quantity no
    //longer inflates the fee
    Account acct(0.50); //too little balance to afford the order below
    std::unordered_map<std::string, Bar> bars; //empty: Broker will look up "AAPL" and get a zero-valued default Bar
    Broker broker(acct, bars);

    Order order;
    order.ticker = "AAPL";
    order.type = "market";
    order.side = 0; //buy
    order.quantity = 500; //costs far more than the $10 balance -> checkOrder fails
    order.checkPrice = -1;

    int id = broker.createOrder(order);
    const Trade& logged = broker.returnHistory()[id];

    REQUIRE(logged.filled == false);
    REQUIRE(logged.side == 0);
}

TEST_CASE("deleteOrder logs the order's actual side (not its quantity) when cancelling a pending order", "[broker]") {
    Account acct(100000.0); //plenty of balance so the order below is accepted as pending
    std::unordered_map<std::string, Bar> bars;
    Broker broker(acct, bars);

    Order order;
    order.ticker = "AAPL";
    order.type = "market";
    order.side = 0; //buy
    order.quantity = 42; //deliberately different from `side` so a mix-up is obvious
    order.checkPrice = -1;

    int id = broker.createOrder(order); //accepted -> lands in the pending orders map
    broker.deleteOrder(id, "test cancellation");
    const Trade& logged = broker.returnHistory()[id];

    //quantity==42 confirms the cancellation record actually landed under
    //`id` (a missing/misfiled record would auto-vivify as a zero-valued
    //Trade here instead, with quantity==0).
    REQUIRE(logged.quantity == 42);
    REQUIRE(logged.side == 0);
}

//--- Baseline coverage: checkOrder's validation rules (not bugs, but worth
//pinning down so a future change can't silently break them) ---

TEST_CASE("checkOrder rejects a buy order when balance is insufficient", "[broker]") {
    //same reasoning as the test above: against a $0-priced bar, the balance
    //must be below the flat commisionFee itself to genuinely fail now that
    //commission is a flat fee instead of being multiplied by quantity
    Account acct(0.50);
    std::unordered_map<std::string, Bar> bars;
    Broker broker(acct, bars);

    Order order;
    order.ticker = "AAPL";
    order.type = "market";
    order.side = 0;
    order.quantity = 100;
    order.checkPrice = -1;

    int id = broker.createOrder(order);

    //.count(id) on an unordered_map is a membership test - it returns how
    //many entries have that key(0 or 1, since ids are unique here), so
    //==0 means "no such id exists in this map". same idiom
    //Account::checkPosition uses internally
    REQUIRE(broker.returnOrders().count(id) == 0); //never accepted as pending
    REQUIRE(broker.returnHistory()[id].filled == false);
}

TEST_CASE("checkOrder accepts a sell order with zero shares held as a new short, given enough cash collateral", "[broker]") {
    //before short-selling support, this exact setup(ample cash, zero
    //shares) was always rejected outright - now that shorting is allowed,
    //it's a perfectly valid way to open one, since the account can afford
    //the 100%-cash-collateral guardrail (see checkOrder's sell branch)
    Account acct(100000.0); //plenty of cash, zero AAPL shares
    std::unordered_map<std::string, Bar> bars; //empty: auto-vivifies a $0 bar, so any cash balance covers it
    Broker broker(acct, bars);

    Order order;
    order.ticker = "AAPL";
    order.type = "market";
    order.side = 1;
    order.quantity = 10;
    order.checkPrice = -1;

    int id = broker.createOrder(order);

    REQUIRE(broker.returnOrders().count(id) == 1); //accepted as a pending short-opening sell
}

TEST_CASE("checkOrder rejects a short-opening sell when there isn't enough cash collateral", "[broker]") {
    Bar bar;
    bar.ticker = "AAPL";
    bar.date = "2024-01-01";
    bar.open = 100.0;
    bar.high = 100.0;
    bar.low = 100.0;
    bar.close = 100.0;
    bar.volume = 1000;

    //shorting 10 shares @ $100 needs $1000 of cash collateral - this
    //account only has $500
    Account acct(500.0);
    std::unordered_map<std::string, Bar> bars{{"AAPL", bar}};
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

//--- Baseline coverage: checkOrderLimitAndStop's trigger conditions, and
//processOrder's execPrice math (slippage + commission), exercised together
//through the public checkLoop() -- checkOrderLimitAndStop and processOrder
//are private, so a pending order + checkLoop() is how the public API
//exercises them. This also covers all three order types' execPrice,
//standing in as the baseline test for the stop-order execPrice question
//investigated during planning (a fallback assignment after the type
//if/else already covers it -- there was no bug to fix, but this guards
//against ever removing that fallback without noticing). ---

TEST_CASE("processOrder fills pending orders via checkLoop with correct execPrice for every order type", "[broker]") {
    Bar bar;
    bar.ticker = "AAPL";
    bar.date = "2024-01-01";
    bar.open = 100.0;
    bar.high = 105.0;
    bar.low = 95.0;
    bar.close = 102.0;
    bar.volume = 1000;

    //declared const since neither value is meant to change across the
    //SECTIONs below - a small guardrail against accidentally reassigning
    //one inside a section and silently changing every other section's math
    const double commission = 1.0;
    const double slippage = 0.01; //1%, chosen for round numbers below

    SECTION("market buy") {
        Account acct(100000.0);
        std::unordered_map<std::string, Bar> bars{{"AAPL", bar}};
        Broker broker(acct, bars, commission, slippage, "b");

        //"Order{...}" aggregate-initializes an Order by listing its fields
        //positionally(ticker, type, side, quantity, checkPrice, in that
        //declaration order) - a shorthand for setting each field on its own
        //line, used throughout this file for orders that don't need every
        //field explained individually
        Order order{"AAPL", "market", 0, 10, -1};
        int id = broker.createOrder(order);
        broker.checkLoop();

        REQUIRE(broker.returnHistory()[id].filled == true);
        //execPrice is now pure slippage -- commisionFee is deducted
        //separately as its own ledger entry, not baked into price per-share
        //open*(1+slip) = 100*1.01 = 101.0
        REQUIRE(broker.returnHistory()[id].execPrice == Catch::Approx(101.0));
    }

    SECTION("market sell") {
        Account acct(100000.0);
        acct.buyNewPosition("AAPL", 50, 90.0); //shares to sell
        std::unordered_map<std::string, Bar> bars{{"AAPL", bar}};
        Broker broker(acct, bars, commission, slippage, "b");

        Order order{"AAPL", "market", 1, 10, -1};
        int id = broker.createOrder(order);
        broker.checkLoop();

        //open*(1-slip) = 100*0.99 = 99.0
        REQUIRE(broker.returnHistory()[id].execPrice == Catch::Approx(99.0));
    }

    SECTION("limit buy triggers when low <= checkPrice") {
        Account acct(100000.0);
        std::unordered_map<std::string, Bar> bars{{"AAPL", bar}};
        Broker broker(acct, bars, commission, slippage, "b");

        Order order{"AAPL", "limit", 0, 10, 98.0}; //low(95) <= 98 -> triggers
        int id = broker.createOrder(order);
        broker.checkLoop();

        REQUIRE(broker.returnHistory()[id].filled == true);
        //min(checkPrice, open)*(1+slip) = min(98,100)*1.01 = 98.98
        REQUIRE(broker.returnHistory()[id].execPrice == Catch::Approx(98.98));
    }

    SECTION("limit sell triggers when high >= checkPrice") {
        Account acct(100000.0);
        acct.buyNewPosition("AAPL", 50, 90.0);
        std::unordered_map<std::string, Bar> bars{{"AAPL", bar}};
        Broker broker(acct, bars, commission, slippage, "b");

        Order order{"AAPL", "limit", 1, 10, 102.0}; //high(105) >= 102 -> triggers
        int id = broker.createOrder(order);
        broker.checkLoop();

        //max(checkPrice, open)*(1-slip) = max(102,100)*0.99 = 100.98
        REQUIRE(broker.returnHistory()[id].execPrice == Catch::Approx(100.98));
    }

    SECTION("stop buy triggers when high >= checkPrice") {
        Account acct(100000.0);
        std::unordered_map<std::string, Bar> bars{{"AAPL", bar}};
        Broker broker(acct, bars, commission, slippage, "b");

        Order order{"AAPL", "stop", 0, 10, 104.0}; //high(105) >= 104 -> triggers
        int id = broker.createOrder(order);
        broker.checkLoop();

        REQUIRE(broker.returnHistory()[id].filled == true);
        //max(checkPrice, open)*(1+slip) = max(104,100)*1.01 = 105.04
        REQUIRE(broker.returnHistory()[id].execPrice == Catch::Approx(105.04));
    }

    SECTION("stop sell triggers when low <= checkPrice") {
        Account acct(100000.0);
        acct.buyNewPosition("AAPL", 50, 90.0);
        std::unordered_map<std::string, Bar> bars{{"AAPL", bar}};
        Broker broker(acct, bars, commission, slippage, "b");

        Order order{"AAPL", "stop", 1, 10, 96.0}; //low(95) <= 96 -> triggers
        int id = broker.createOrder(order);
        broker.checkLoop();

        //min(checkPrice, open)*(1-slip) = min(96,100)*0.99 = 95.04
        REQUIRE(broker.returnHistory()[id].execPrice == Catch::Approx(95.04));
    }
}

TEST_CASE("stop_limit orders trigger like a plain stop but fill as a limit at limitPrice", "[broker]") {
    Bar bar;
    bar.ticker = "AAPL";
    bar.date = "2024-01-01";
    bar.high = 110.0;
    bar.low = 90.0;
    bar.close = 100.0;
    bar.volume = 1000;

    const double commission = 0.0;
    const double slippage = 0.0;

    SECTION("open hasn't gapped past the limit - fill is bounded by open") {
        bar.open = 100.0;
        Account acct(100000.0);
        std::unordered_map<std::string, Bar> bars{{"AAPL", bar}};
        Broker broker(acct, bars, commission, slippage, "b");

        //checkPrice(95) is the stop TRIGGER - high(110) >= 95 fires it, same
        //trigger checkOrderLimitAndStop already uses for a plain stop.
        //limitPrice(105) is only checked once triggered
        Order order{"AAPL", "stop_limit", 0, 10, 95.0, 105.0};
        int id = broker.createOrder(order);
        broker.checkLoop();

        REQUIRE(broker.returnHistory()[id].filled == true);
        //min(limitPrice, open) = min(105,100) = 100
        REQUIRE(broker.returnHistory()[id].execPrice == Catch::Approx(100.0));
    }

    SECTION("open has gapped past the limit - fill is capped at limitPrice") {
        bar.open = 108.0; //gapped above the 105 limit
        Account acct(100000.0);
        std::unordered_map<std::string, Bar> bars{{"AAPL", bar}};
        Broker broker(acct, bars, commission, slippage, "b");

        Order order{"AAPL", "stop_limit", 0, 10, 95.0, 105.0};
        int id = broker.createOrder(order);
        broker.checkLoop();

        REQUIRE(broker.returnHistory()[id].filled == true);
        //min(limitPrice, open) = min(105,108) = 105 - capped at the limit,
        //demonstrably different from a plain stop, which would fill at
        //max(checkPrice, open) = max(95,108) = 108 instead
        REQUIRE(broker.returnHistory()[id].execPrice == Catch::Approx(105.0));
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

    Order order{"AAPL", "limit", 0, 10, 50.0}; //low(95) > 50 -> never triggers on this bar
    int id = broker.createOrder(order);
    broker.checkLoop();

    REQUIRE(broker.returnOrders().count(id) == 1); //still pending
    REQUIRE(broker.returnHistory().count(id) == 0); //no trade recorded yet
}

TEST_CASE("processOrder computes realizedPnL for a sell relative to the pre-sale average entry price", "[broker]") {
    Account acct(100000.0);
    acct.buyNewPosition("AAPL", 50, 90.0); //50 shares @ AEP 90.0

    Bar bar;
    bar.ticker = "AAPL";
    bar.date = "2024-01-01";
    bar.open = 100.0;
    bar.high = 100.0;
    bar.low = 100.0;
    bar.close = 100.0;
    bar.volume = 1000;

    std::unordered_map<std::string, Bar> bars{{"AAPL", bar}};
    Broker broker(acct, bars, /*commission*/0.0, /*slippage*/0.0, "b");

    SECTION("partial sell") {
        Order order{"AAPL", "market", 1, 20, -1};
        int id = broker.createOrder(order);
        broker.checkLoop();

        //execPrice = open*(1-0) - 0 = 100.0; realizedPnL = (100-90)*20 = 200.0
        REQUIRE(broker.returnHistory()[id].realizedPnL == Catch::Approx(200.0));
    }

    SECTION("sell all") {
        Order order{"AAPL", "market", 1, 50, -1};
        int id = broker.createOrder(order);
        broker.checkLoop();

        //realizedPnL = (100-90)*50 = 500.0
        REQUIRE(broker.returnHistory()[id].realizedPnL == Catch::Approx(500.0));
    }
}

TEST_CASE("a full short-then-cover cycle realizes P&L correctly through checkLoop", "[broker]") {
    Bar bar;
    bar.ticker = "AAPL";
    bar.date = "2024-01-01";
    bar.open = 100.0;
    bar.high = 100.0;
    bar.low = 100.0;
    bar.close = 100.0;
    bar.volume = 1000;

    Account acct(100000.0);
    std::unordered_map<std::string, Bar> bars{{"AAPL", bar}};
    Broker broker(acct, bars, /*commission*/0.0, /*slippage*/0.0, "b");

    //open a short from flat: sell 50 shares with none currently held
    Order shortOrder{"AAPL", "market", 1, 50, -1};
    int shortId = broker.createOrder(shortOrder);
    broker.checkLoop();

    REQUIRE(broker.returnHistory()[shortId].filled == true);
    REQUIRE(acct.positionQuantity("AAPL") == -50);
    REQUIRE(acct.positionAEP("AAPL") == Catch::Approx(100.0));

    SECTION("covering at a lower price realizes a profit") {
        bars["AAPL"].open = 80.0; //price fell - a short profits when covered lower
        Order coverOrder{"AAPL", "market", 0, 50, -1};
        int coverId = broker.createOrder(coverOrder);
        broker.checkLoop();

        //(preBuyAEP - currPrice)*coveredQty = (100-80)*50 = 1000.0
        REQUIRE(broker.returnHistory()[coverId].realizedPnL == Catch::Approx(1000.0));
        REQUIRE(acct.positionQuantity("AAPL") == 0);
    }

    SECTION("covering at a higher price realizes a loss") {
        bars["AAPL"].open = 110.0; //price rose - a short loses when covered higher
        Order coverOrder{"AAPL", "market", 0, 50, -1};
        int coverId = broker.createOrder(coverOrder);
        broker.checkLoop();

        //(100-110)*50 = -500.0
        REQUIRE(broker.returnHistory()[coverId].realizedPnL == Catch::Approx(-500.0));
        REQUIRE(acct.positionQuantity("AAPL") == 0);
    }
}
