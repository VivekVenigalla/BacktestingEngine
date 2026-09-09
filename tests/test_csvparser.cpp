#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "csvParser.hpp"
#include "projectPaths.hpp"

//Parser::parse() reads columns positionally (Date,Open,High,Low,Close,Volume)
//with no header validation. This test just pins down that the happy path
//produces exactly the Bar values you'd expect from a small, hand-written
//fixture file -- a baseline so any future change to the parsing logic has
//something to check itself against.
TEST_CASE("parse reads a CSV file into correctly-valued Bars keyed by date", "[csvparser]") {
    Parser parser;
    std::string path = std::string(TEST_FIXTURES_DIR) + "/sample_bars.csv";
    std::map<std::string, Bar> bars = parser.parse("AAPL", path);

    REQUIRE(bars.size() == 3);

    //bars.at("2024-01-01") looks up the map entry, throwing if it's
    //missing(unlike operator[], which would silently insert a blank
    //entry) - the right choice here since a missing key would itself be a
    //test failure worth seeing, not something to paper over
    const Bar& first = bars.at("2024-01-01");
    REQUIRE(first.ticker == "AAPL");
    REQUIRE(first.open == Catch::Approx(100.0));
    REQUIRE(first.high == Catch::Approx(105.0));
    REQUIRE(first.low == Catch::Approx(95.0));
    REQUIRE(first.close == Catch::Approx(102.0));
    REQUIRE(first.volume == 1000);

    const Bar& last = bars.at("2024-01-03");
    REQUIRE(last.open == Catch::Approx(108.0));
    REQUIRE(last.high == Catch::Approx(109.0));
    REQUIRE(last.low == Catch::Approx(103.0));
    REQUIRE(last.close == Catch::Approx(104.0));
    REQUIRE(last.volume == 1200);
}
