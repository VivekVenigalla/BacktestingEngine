#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <queue>
#include "strategies/bollBand.hpp"
#include "broker.hpp"
#include "account.hpp"

// standardDeviation's accumulator was declared `double temp;` with no
// initializer, then immediately used with `+=` -- undefined behavior, since
// its starting value is whatever garbage happened to be on the stack. This
// test uses a small series whose sample standard deviation can be checked
// by hand, so a wrong (or unstable) result is easy to spot.
TEST_CASE("standardDeviation computes the correct sample standard deviation", "[bollband]") {
    Account acct(100000.0);
    std::unordered_map<std::string, Bar> bars;
    std::unordered_map<long int, Trade> history;
    Broker broker(acct, bars);

    bollBand strat(broker, acct, bars, history, "AAPL", 8);

    std::queue<double> series;
    for (double v : {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0}) {
        series.push(v);
    }
    double average = 5.0; // (2+4+4+4+5+5+7+9)/8

    // Sample standard deviation (n-1 divisor) of this series is ~2.1380899.
    REQUIRE(strat.standardDeviation(series, average) == Catch::Approx(2.1380899).margin(0.0001));
}
