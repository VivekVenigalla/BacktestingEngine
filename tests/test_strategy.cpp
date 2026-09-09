#include <catch2/catch_test_macros.hpp>
#include "strategy.hpp"

// Strategy is abstract, and sizeBuyOrder/sizeSellOrder are `protected` (meant
// for concrete strategies' own runBar() logic, not a public API) -- this
// minimal subclass exists purely to give a test access to them, via public
// wrapper methods. runBar()/init() are unused no-ops here.
class TestableStrategy : public Strategy {
    public:
        TestableStrategy(Broker& b, Account& u, std::unordered_map<std::string, Bar>& cBs, std::unordered_map<long int, Trade>& history, std::string symbol, double posSizePct)
            : Strategy(b, u, cBs, history, symbol) {
            positionSizePct = posSizePct;
        }
        void runBar() override {}
        void init() override {}

        long testSizeBuyOrder(double price) const { return sizeBuyOrder(price); }
        long testSizeSellOrder(long currentQuantity) const { return sizeSellOrder(currentQuantity); }
};

TEST_CASE("sizeBuyOrder and sizeSellOrder use the default 20% position size", "[strategy]") {
    Account acct(10000.0);
    std::unordered_map<std::string, Bar> bars;
    std::unordered_map<long int, Trade> history;
    Broker broker(acct, bars);

    TestableStrategy strat(broker, acct, bars, history, "AAPL", 0.2);

    // floor(10000 * 0.2 / 100) = 20
    REQUIRE(strat.testSizeBuyOrder(100.0) == 20);
    // floor(40 * 0.2) = 8
    REQUIRE(strat.testSizeSellOrder(40) == 8);
}

TEST_CASE("sizeBuyOrder and sizeSellOrder respect a custom position size", "[strategy]") {
    Account acct(10000.0);
    std::unordered_map<std::string, Bar> bars;
    std::unordered_map<long int, Trade> history;
    Broker broker(acct, bars);

    TestableStrategy strat(broker, acct, bars, history, "AAPL", 0.5);

    // floor(10000 * 0.5 / 100) = 50
    REQUIRE(strat.testSizeBuyOrder(100.0) == 50);
    // floor(40 * 0.5) = 20
    REQUIRE(strat.testSizeSellOrder(40) == 20);
}
