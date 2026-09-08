#include <catch2/catch_test_macros.hpp>
#include "dataFeed.hpp"
#include "projectPaths.hpp"

TEST_CASE("Data streams bars in date order and reports when it's exhausted", "[datafeed]") {
    std::string path = std::string(TEST_FIXTURES_DIR) + "/sample_bars.csv";
    Data feed("fixture_feed", "AAPL", path);

    REQUIRE(feed.totalBars() == 3);
    REQUIRE(feed.hasMoreData());
    REQUIRE(feed.getBar().date == "2024-01-01");

    feed.nextBar();
    REQUIRE(feed.hasMoreData());
    REQUIRE(feed.getBar().date == "2024-01-02");

    feed.nextBar();
    REQUIRE(feed.hasMoreData());
    REQUIRE(feed.getBar().date == "2024-01-03");

    feed.nextBar();
    REQUIRE_FALSE(feed.hasMoreData()); // advanced past the last bar

    feed.reset();
    REQUIRE(feed.hasMoreData());
    REQUIRE(feed.getBar().date == "2024-01-01");
}
