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
