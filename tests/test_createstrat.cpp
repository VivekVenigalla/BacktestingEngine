#include <catch2/catch_test_macros.hpp>
#include "createStrat.hpp"
#include "strategies/smaCross.hpp"
#include "strategies/bollBand.hpp"
#include "strategies/donChannel.hpp"

TEST_CASE("StrategyFactory::create builds the concrete strategy matching the requested type", "[createstrat]") {
    Account acct(100000.0);
    std::unordered_map<std::string, Bar> bars;
    std::unordered_map<long int, Trade> history;
    Broker broker(acct, bars);
    std::vector<std::string> symbols{"AAPL"};

    SECTION("sma") {
        json params = {{"fast_period", 10}, {"slow_period", 30}};
        auto strat = StrategyFactory::create(broker, acct, bars, history, symbols, "sma", params);
        REQUIRE(dynamic_cast<smaCross*>(strat.get()) != nullptr);
    }

    SECTION("boll") {
        json params = {{"window", 20}};
        auto strat = StrategyFactory::create(broker, acct, bars, history, symbols, "boll", params);
        REQUIRE(dynamic_cast<bollBand*>(strat.get()) != nullptr);
    }

    SECTION("don") {
        json params = {{"window", 20}};
        auto strat = StrategyFactory::create(broker, acct, bars, history, symbols, "don", params);
        REQUIRE(dynamic_cast<donChannel*>(strat.get()) != nullptr);
    }
}

TEST_CASE("StrategyFactory::create threads position_size_pct through to the sizing helper", "[createstrat]") {
    Account acct(10000.0);
    std::unordered_map<std::string, Bar> bars;
    std::unordered_map<long int, Trade> history;
    Broker broker(acct, bars);
    std::vector<std::string> symbols{"AAPL"};

    json params = {{"fast_period", 2}, {"slow_period", 4}, {"position_size_pct", 0.5}};
    auto strat = StrategyFactory::create(broker, acct, bars, history, symbols, "sma", params);

    // Same golden-cross series as test_smacross.cpp's golden-cross test
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
        strat->runBar();
    }

    REQUIRE(broker.returnOrders().size() == 1);
    const Order& placed = broker.returnOrders().begin()->second;
    // floor(10000 * 0.5 / 140) = 35 -- vs. floor(10000*0.2/140)=14 at the
    // default 20%, confirming the config value actually changed the sizing.
    REQUIRE(placed.quantity == 35);
}

TEST_CASE("StrategyFactory::create throws for an unrecognized strategy type", "[createstrat]") {
    Account acct(100000.0);
    std::unordered_map<std::string, Bar> bars;
    std::unordered_map<long int, Trade> history;
    Broker broker(acct, bars);
    std::vector<std::string> symbols{"AAPL"};
    json params = json::object();

    REQUIRE_THROWS_AS(
        StrategyFactory::create(broker, acct, bars, history, symbols, "totally_unknown", params),
        std::invalid_argument
    );
}
