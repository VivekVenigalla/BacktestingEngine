#include <catch2/catch_test_macros.hpp>
#include "dataFeed.hpp"
#include "projectPaths.hpp"

//TEST_CASE("description", "[tag]") declares one independent, self-contained
//test - catch2(the testing library this project uses) discovers every
//TEST_CASE automatically at build time and runs each one as its own
//separate entry when you run ctest. the description string is just for
//humans(it's what prints if this test fails), and the "[datafeed]" tag
//lets you filter down to just this file's tests later, e.g running
//./build/tests/unit_tests "[datafeed]" instead of the whole suite
TEST_CASE("Data streams bars in date order and reports when it's exhausted", "[datafeed]") {
    //TEST_FIXTURES_DIR is a build-time constant(see include/projectPaths.hpp.in)
    //pointing at tests/fixtures - using it instead of a relative path like
    //"../tests/fixtures" means this test finds its csv file no matter what
    //directory ctest happens to be run from
    std::string path = std::string(TEST_FIXTURES_DIR) + "/sample_bars.csv";
    Data feed("fixture_feed", "AAPL", path);

    //REQUIRE(expression) is the actual assertion - if expression evaluates
    //to false, this specific TEST_CASE is marked failed right here and
    //stops immediately(the rest of the function body doesn't run), and
    //catch2 prints exactly which REQUIRE failed with both sides of the
    //comparison expanded out, without needing a custom error message
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
    REQUIRE_FALSE(feed.hasMoreData()); //advanced past the last bar, REQUIRE_FALSE(x) is shorthand for REQUIRE(!x)

    feed.reset();
    REQUIRE(feed.hasMoreData());
    REQUIRE(feed.getBar().date == "2024-01-01");
}
