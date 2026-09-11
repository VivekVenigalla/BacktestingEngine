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

//--- Short-selling: a negative Position::quantity means "short" throughout
//this codebase. buyNewPosition is already sign-agnostic (confirmed during
//Phase 3 planning - "balance -= quantity*entryPrice" correctly CREDITS
//cash for a negative quantity), so it doubles as "open a short directly"
//in the tests below, standing in for whatever earlier trade actually put
//the account short in a real run ---

TEST_CASE("sellPositionQuantity opens a short from flat and sets its AEP to the fill price", "[account]") {
    Account acct(10000.0);
    acct.sellPositionQuantity("AAPL", 50, 100.0); //balance: 10000 + 50*100 = 15000

    REQUIRE(acct.positionQuantity("AAPL") == -50);
    REQUIRE(acct.positionAEP("AAPL") == Catch::Approx(100.0));
    REQUIRE(acct.checkBalance() == Catch::Approx(15000.0));
}

TEST_CASE("sellPositionQuantity extending an existing short blends the new shares into its AEP", "[account]") {
    Account acct(10000.0);
    acct.buyNewPosition("AAPL", -50, 100.0);      //short 50 @ AEP 100, balance: 10000+5000=15000
    acct.sellPositionQuantity("AAPL", 20, 110.0); //extends the short by 20 more @ 110

    REQUIRE(acct.positionQuantity("AAPL") == -70);
    //weighted like a long's AEP, just by short size: (50*100+20*110)/70
    REQUIRE(acct.positionAEP("AAPL") == Catch::Approx(102.857142857).margin(0.0001));
    REQUIRE(acct.checkBalance() == Catch::Approx(17200.0));
}

TEST_CASE("buyPositionQuantity partially covers a short and leaves its AEP unchanged", "[account]") {
    Account acct(10000.0);
    acct.buyNewPosition("AAPL", -50, 100.0);     //short 50 @ AEP 100, balance: 15000
    acct.buyPositionQuantity("AAPL", 20, 90.0);  //covers 20 of the 50 short shares

    REQUIRE(acct.positionQuantity("AAPL") == -30);
    //covering part of a short doesn't touch the AEP of the shares still
    //held short, same principle as partially selling a long
    REQUIRE(acct.positionAEP("AAPL") == Catch::Approx(100.0));
    REQUIRE(acct.checkBalance() == Catch::Approx(13200.0)); //15000 - 20*90
}

TEST_CASE("buyPositionQuantity covering a short past zero opens a fresh long at the fill price", "[account]") {
    Account acct(10000.0);
    acct.buyNewPosition("AAPL", -50, 100.0);     //short 50 @ AEP 100, balance: 15000
    acct.buyPositionQuantity("AAPL", 70, 90.0);  //covers all 50 short shares + opens a 20-share long

    REQUIRE(acct.positionQuantity("AAPL") == 20);
    //the short contributed nothing to the new long's cost basis
    REQUIRE(acct.positionAEP("AAPL") == Catch::Approx(90.0));
    REQUIRE(acct.checkBalance() == Catch::Approx(8700.0)); //15000 - 70*90
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
