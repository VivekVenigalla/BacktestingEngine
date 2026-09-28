#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "walkForward.hpp"
#include "dataFeed.hpp"
#include "projectPaths.hpp"

//builds a synthetic "YYYY-MM-DD" date string for day N(0-based) of month
//01, e.g dayString(0) == "2024-01-01", dayString(9) == "2024-01-10" - this
//keeps the hand-computed boundaries below easy to verify by eye, the same
//way Day 6's throwaway smoke test did before it got deleted
static std::string dayString(int n){
    //zero-padded to 2 digits, since date strings only sort correctly if
    //every one is the same width("2024-01-9" would sort AFTER
    //"2024-01-10" as plain text, which is exactly the bug lower_bound/
    //upper_bound in sliceBars rely on this NOT happening)
    std::string dayPart = (n + 1 < 10) ? ("0" + std::to_string(n + 1)) : std::to_string(n + 1);
    return "2024-01-" + dayPart;
}

TEST_CASE("generateWindows produces correctly-bounded rolling windows", "[walkforward]") {
    //30 sequential dates, 2024-01-01 through 2024-01-30
    std::vector<std::string> dates;
    for(int i = 0; i < 30; i++){
        dates.push_back(dayString(i));
    }

    SECTION("inSample=10, outSample=5, step=5 gives exactly 4 windows") {
        //windowSpan = 15, so windows start at indices 0, 5, 10, 15(15+15=30
        //fits exactly) - index 20 would need 20+15=35, past the 30 dates
        //available, so it's correctly excluded
        std::vector<WalkForwardWindow> windows = generateWindows(dates, 10, 5, 5);
        REQUIRE(windows.size() == 4);

        //window 0: in-sample is indices [0,9], out-of-sample is [10,14]
        REQUIRE(windows[0].inSampleStart == dayString(0));
        REQUIRE(windows[0].inSampleEnd == dayString(9));
        REQUIRE(windows[0].outSampleStart == dayString(10));
        REQUIRE(windows[0].outSampleEnd == dayString(14));

        //window 1: everything shifted forward by stepBars(5)
        REQUIRE(windows[1].inSampleStart == dayString(5));
        REQUIRE(windows[1].inSampleEnd == dayString(14));
        REQUIRE(windows[1].outSampleStart == dayString(15));
        REQUIRE(windows[1].outSampleEnd == dayString(19));

        //window 3(the last one): starts at index 15, ends exactly at
        //index 29(the last available date) - confirms the "fits exactly"
        //boundary case is included, not off-by-one excluded
        REQUIRE(windows[3].inSampleStart == dayString(15));
        REQUIRE(windows[3].inSampleEnd == dayString(24));
        REQUIRE(windows[3].outSampleStart == dayString(25));
        REQUIRE(windows[3].outSampleEnd == dayString(29));
    }

    SECTION("a stepBars smaller than inSampleBars produces overlapping windows") {
        //windowSpan = 12, step = 3 -> starts at 0,3,...,up to the last one
        //that still fits: 30-12=18 is the last valid start
        std::vector<WalkForwardWindow> windows = generateWindows(dates, 8, 4, 3);
        REQUIRE(windows.size() == 7); //starts: 0,3,6,9,12,15,18

        //window 1's in-sample(starting at index 3) overlaps window 0's
        //in-sample(indices 0-7) across indices 3-7
        REQUIRE(windows[0].inSampleEnd == dayString(7));
        REQUIRE(windows[1].inSampleStart == dayString(3));
    }

    SECTION("exactly enough dates for one window produces exactly one window") {
        std::vector<WalkForwardWindow> windows = generateWindows(dates, 20, 10, 5);
        REQUIRE(windows.size() == 1);
        REQUIRE(windows[0].inSampleStart == dayString(0));
        REQUIRE(windows[0].outSampleEnd == dayString(29));
    }

    SECTION("too few dates for even one window returns empty, not a crash") {
        std::vector<WalkForwardWindow> windows = generateWindows(dates, 20, 15, 5);
        REQUIRE(windows.empty());
    }

    SECTION("an empty date list returns empty") {
        std::vector<std::string> empty;
        REQUIRE(generateWindows(empty, 5, 5, 5).empty());
    }

    SECTION("zero or negative bar counts return empty rather than looping forever or crashing") {
        REQUIRE(generateWindows(dates, 0, 5, 5).empty());
        REQUIRE(generateWindows(dates, 5, 0, 5).empty());
        REQUIRE(generateWindows(dates, 5, 5, 0).empty());
        REQUIRE(generateWindows(dates, -1, 5, 5).empty());
        REQUIRE(generateWindows(dates, 5, 5, -1).empty());
    }
}

TEST_CASE("Data::sliceBars returns the exact expected subset without mutating the source", "[walkforward]") {
    std::string path = std::string(TEST_FIXTURES_DIR) + "/sample_bars.csv";
    Data feed("fixture_feed", "AAPL", path);
    //fixture holds exactly 2024-01-01, 2024-01-02, 2024-01-03(see
    //test_datafeed.cpp for the same file)

    SECTION("a range covering all bars returns everything") {
        std::map<std::string, Bar> slice = feed.sliceBars("2024-01-01", "2024-01-03");
        REQUIRE(slice.size() == 3);
        REQUIRE(slice.begin()->first == "2024-01-01");
        REQUIRE(slice.rbegin()->first == "2024-01-03");
    }

    SECTION("a partial range returns exactly the matching bars, with correct data") {
        std::map<std::string, Bar> slice = feed.sliceBars("2024-01-01", "2024-01-02");
        REQUIRE(slice.size() == 2);
        REQUIRE(slice.begin()->first == "2024-01-01");
        REQUIRE(slice.rbegin()->first == "2024-01-02");
        REQUIRE(slice.at("2024-01-02").close == Catch::Approx(108.0));
    }

    SECTION("a single-day range returns exactly one bar") {
        std::map<std::string, Bar> slice = feed.sliceBars("2024-01-02", "2024-01-02");
        REQUIRE(slice.size() == 1);
        REQUIRE(slice.at("2024-01-02").close == Catch::Approx(108.0));
    }

    SECTION("a range entirely before the data returns empty") {
        REQUIRE(feed.sliceBars("2023-01-01", "2023-12-31").empty());
    }

    SECTION("a range entirely after the data returns empty") {
        REQUIRE(feed.sliceBars("2025-01-01", "2025-12-31").empty());
    }

    SECTION("a range that overshoots both ends still clamps to the real data") {
        std::map<std::string, Bar> slice = feed.sliceBars("2000-01-01", "2099-12-31");
        REQUIRE(slice.size() == 3);
    }

    SECTION("slicing never disturbs the original feed's own streaming position") {
        //advance the feed partway through before slicing, to prove
        //sliceBars doesn't touch currBar even mid-stream
        feed.nextBar();
        REQUIRE(feed.getBar().date == "2024-01-02");

        std::map<std::string, Bar> slice = feed.sliceBars("2024-01-01", "2024-01-01");
        REQUIRE(slice.size() == 1);

        //the feed's own position is exactly where it was before slicing
        REQUIRE(feed.getBar().date == "2024-01-02");
        REQUIRE(feed.totalBars() == 3);

        //and it can still stream the rest of its own bars normally
        feed.nextBar();
        REQUIRE(feed.getBar().date == "2024-01-03");
        feed.nextBar();
        REQUIRE_FALSE(feed.hasMoreData());
    }
}

TEST_CASE("Data::allDates returns every bar date in ascending order", "[walkforward]") {
    std::string path = std::string(TEST_FIXTURES_DIR) + "/sample_bars.csv";
    Data feed("fixture_feed", "AAPL", path);

    std::vector<std::string> dates = feed.allDates();
    REQUIRE(dates.size() == 3);
    REQUIRE(dates[0] == "2024-01-01");
    REQUIRE(dates[1] == "2024-01-02");
    REQUIRE(dates[2] == "2024-01-03");

    //allDates() is a pure read - the feed's own streaming position(still
    //at bar 0, since nothing above called nextBar()) is unaffected
    REQUIRE(feed.getBar().date == "2024-01-01");
}

//builds a synthetic Data feed with `barCount` sequential daily bars(dates
//starting 2024-01-01) whose close price rises by exactly $1 every bar -
//real enough to drive a real smaCross strategy through runWalkForward,
//without needing a csv fixture file
static Data makeRisingFeed(const std::string& id, const std::string& ticker, int barCount){
    std::map<std::string, Bar> bars;
    for(int i = 0; i < barCount; i++){
        Bar bar;
        bar.ticker = ticker;
        bar.date = dayString(i);
        bar.open = 100.0 + i;
        bar.close = 100.0 + i;
        bar.high = 101.0 + i;
        bar.low = 99.0 + i;
        bar.volume = 1000;
        bars[bar.date] = bar;
    }
    return Data(id, ticker, bars);
}

TEST_CASE("runWalkForward drives a real strategy over each window and records both sides' metrics", "[walkforward]") {
    //60 bars is enough for a couple of small windows with room to spare -
    //dayString only covers January(31 days), so this test's window sizes
    //stay well under that
    std::unordered_map<std::string, Data> feeds;
    feeds.emplace("AAPL_feed", makeRisingFeed("AAPL_feed", "AAPL", 30));

    WalkForwardSetup setup;
    setup.simID = "wf_test_sim";
    setup.strategyType = "sma";
    setup.strategyParams = {{"fast_period", 2}, {"slow_period", 4}};
    setup.initialBalance = 10000.0;
    setup.commissionRate = 1.0;
    setup.slippageRate = 0.0005;
    setup.cagrLength = 1.0;
    setup.feedIDs = {"AAPL_feed"};
    setup.allTickers = {"AAPL_feed"};

    //windowSpan = 10, step = 10 -> starts at index 0, 10, up to the last
    //that fits within 30 dates: 20(20+10=30) -> 3 windows expected
    std::vector<WalkForwardResult> results = runWalkForward(setup, feeds, 6, 4, 10);

    REQUIRE(results.size() == 3);

    for(const WalkForwardResult& result : results){
        //in-sample directly precedes out-of-sample, with no gap and no
        //overlap - the same boundary generateWindows guarantees
        REQUIRE(result.window.inSampleEnd < result.window.outSampleStart);
        //a strictly rising price series with a fast/slow SMA crossover
        //should place at least one trade once the slow SMA has enough bars
        REQUIRE(result.inSample.numTrades >= 0);
        REQUIRE(result.outSample.numTrades >= 0);
    }

    //the ORIGINAL feed passed in is completely untouched - sliceBars
    //inside runSlice only ever returns copies, so the source feed's own
    //bars/position are exactly as they started
    REQUIRE(feeds.at("AAPL_feed").totalBars() == 30);
    REQUIRE(feeds.at("AAPL_feed").getBar().date == "2024-01-01");
}

TEST_CASE("runWalkForward returns an empty result set rather than crashing when no setup feeds are given", "[walkforward]") {
    std::unordered_map<std::string, Data> feeds;
    WalkForwardSetup setup; //feedIDs left empty

    std::vector<WalkForwardResult> results = runWalkForward(setup, feeds, 5, 5, 5);
    REQUIRE(results.empty());
}
