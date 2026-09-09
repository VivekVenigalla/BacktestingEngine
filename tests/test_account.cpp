#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "account.hpp"

//TEST_CASE declares one independent test. The string is a description
//(shown in ctest/Catch2 output on failure); the "[account]" tag lets you
//run just this file's tests later with e.g. `./unit_tests "[account]"`.
//re-using the same tag across every TEST_CASE in a file(as below) is the
//normal convention here - it groups a whole file's worth of tests under
//one filterable name, matching the pattern every other test file uses too

TEST_CASE("buyPositionQuantity computes a quantity-weighted average entry price", "[account]") {
    Account acct(100000.0);
    acct.buyNewPosition("AAPL", 10, 100.0);
    acct.buyPositionQuantity("AAPL", 30, 120.0);

    //Weighted average: (10*100 + 30*120) / (10+30) = 115.0
    //The naive (old + new) / 2 average would give 110.0 instead.
    REQUIRE(acct.positionAEP("AAPL") == Catch::Approx(115.0));
}

TEST_CASE("sellPositionQuantity reduces quantity and credits balance", "[account]") {
    Account acct(10000.0);
    acct.buyNewPosition("AAPL", 50, 100.0);       //balance: 10000 - 5000 = 5000
    acct.sellPositionQuantity("AAPL", 20, 110.0); //balance: 5000 + 2200 = 7200

    REQUIRE(acct.positionQuantity("AAPL") == 30);
    REQUIRE(acct.checkBalance() == Catch::Approx(7200.0));
}

TEST_CASE("sellAllPosition zeroes quantity and credits full proceeds", "[account]") {
    Account acct(10000.0);
    acct.buyNewPosition("AAPL", 50, 100.0); //balance: 10000 - 5000 = 5000
    acct.sellAllPosition("AAPL", 120.0);    //balance: 5000 + 50*120 = 11000

    REQUIRE(acct.positionQuantity("AAPL") == 0);
    REQUIRE(acct.checkBalance() == Catch::Approx(11000.0));
}

TEST_CASE("accountValue sums cash and multiple ticker positions", "[account]") {
    Account acct(10000.0);
    acct.buyNewPosition("AAPL", 10, 100.0); //balance: 10000 - 1000 = 9000
    acct.buyNewPosition("MSFT", 5, 200.0);  //balance: 9000 - 1000 = 8000

    //this brace-initializes an unordered_map directly with two entries -
    //equivalent to default-constructing it and then doing two separate
    //prices["AAPL"]=110.0/prices["MSFT"]=210.0 assignments, just in one line
    std::unordered_map<std::string, double> prices{{"AAPL", 110.0}, {"MSFT", 210.0}};
    //8000 (cash) + 10*110 (AAPL) + 5*210 (MSFT) = 10150
    REQUIRE(acct.accountValue(prices) == Catch::Approx(10150.0));
}

TEST_CASE("reset restores the initial balance and zeroes position quantities", "[account]") {
    //Only the 3-arg constructor records an `initial` balance for reset() to
    //restore to -- the 1-arg constructor never sets it (it stays 0.0).
    Account acct(10000.0, std::vector<std::string>{"AAPL"}, "acct1");
    acct.buyPositionQuantity("AAPL", 10, 100.0);
    acct.reset();

    REQUIRE(acct.checkBalance() == Catch::Approx(10000.0));
    REQUIRE(acct.positionQuantity("AAPL") == 0);
}
