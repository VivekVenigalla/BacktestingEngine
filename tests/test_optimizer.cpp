#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <set>
#include <sstream>
#include "optimizer.hpp"
#include "dataFeed.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace {

//removes whatever path it holds when it goes out of scope, even if a
//REQUIRE fails partway through(see test_logger.cpp's TempPathGuard for the
//longer explanation of this RAII idea - kept in an anonymous namespace here
//so the two helpers can never collide at link time)
struct TempFileGuard {
    fs::path path;
    ~TempFileGuard() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
};

//a path inside the system temp directory, unique per test name
fs::path tempFilePath(const std::string& name){
    return fs::temp_directory_path() / ("optimizer_test_" + name);
}

std::vector<std::string> readLines(const fs::path& path){
    std::ifstream file(path);
    std::vector<std::string> lines;
    std::string line;
    while(std::getline(file, line)){
        lines.push_back(line);
    }
    return lines;
}

//builds a result by hand, so ranking/export tests don't need to run a
//real simulation to get something to rank
OptimizationResult makeResult(const json& params, double totalReturn, double cagr, double sharpe){
    OptimizationResult result;
    result.parameters = params;
    result.metrics.totalReturn = totalReturn;
    result.metrics.cagr = cagr;
    result.metrics.sharpeRatio = sharpe;
    result.metrics.maxDrawdown = -10.0;
    result.metrics.numTrades = 5;
    return result;
}

//a deterministic synthetic daily feed of `barCount` bars: a steady uptrend
//with a repeating wave on top, so a breakout strategy sees real new highs
//and real pullbacks(a perfectly straight line, or pure noise, would make
//every donChannel window size behave the same). dates are "2024-MM-DD"
//with 28 days per month, which sorts lexicographically in the same order
//as chronologically, like real dates
Data makeWavyFeed(const std::string& id, const std::string& ticker, int barCount){
    std::map<std::string, Bar> bars;
    for(int i = 0; i < barCount; i++){
        int month = i / 28 + 1;
        int day = i % 28 + 1;
        std::string date = "2024-" + std::string(month < 10 ? "0" : "") + std::to_string(month)
                         + "-" + std::string(day < 10 ? "0" : "") + std::to_string(day);

        double price = 100.0 + 0.3 * i + 8.0 * std::sin(i / 4.0);

        Bar bar;
        bar.ticker = ticker;
        bar.date = date;
        bar.open = price;
        bar.close = price;
        bar.high = price + 1.0;
        bar.low = price - 1.0;
        bar.volume = 1000;
        bars[date] = bar;
    }
    return Data(id, ticker, bars);
}

OptimizerSetup makeDonSetup(){
    OptimizerSetup setup;
    setup.simID = "optimizer_test";
    setup.strategyType = "don";
    setup.initialBalance = 100000.0;
    setup.commissionRate = 1.0;
    setup.slippageRate = 0.0005;
    setup.cagrLength = 1.0;
    setup.feedIDs = {"WAVY"};
    setup.allTickers = {"WAVY"};
    return setup;
}

} //namespace

TEST_CASE("ParameterGrid generates the full Cartesian product of its parameters", "[optimizer]") {
    SECTION("a 2-parameter x 3-value grid produces exactly 3 x 3 = 9 distinct combos") {
        ParameterGrid grid;
        grid.addParameter("fast_period", {5, 10, 20});
        grid.addParameter("slow_period", {30, 50, 100});

        std::vector<json> combos = grid.generateCombinations();
        REQUIRE(combos.size() == 9);

        std::set<std::string> distinct;
        for(const json& combo : combos){
            REQUIRE(combo.size() == 2);
            REQUIRE(combo.contains("fast_period"));
            REQUIRE(combo.contains("slow_period"));
            distinct.insert(combo.dump());
        }
        REQUIRE(distinct.size() == 9);

        //spot-check the two corners of the grid
        REQUIRE(combos.front() == json({{"fast_period", 5}, {"slow_period", 30}}));
        REQUIRE(combos.back() == json({{"fast_period", 20}, {"slow_period", 100}}));
    }

    SECTION("the LAST parameter added varies fastest, like the innermost of nested loops") {
        ParameterGrid grid;
        grid.addParameter("a", {1, 2});
        grid.addParameter("b", {10, 20, 30});

        std::vector<json> combos = grid.generateCombinations();
        REQUIRE(combos.size() == 6);
        REQUIRE(combos[0] == json({{"a", 1}, {"b", 10}}));
        REQUIRE(combos[1] == json({{"a", 1}, {"b", 20}}));
        REQUIRE(combos[2] == json({{"a", 1}, {"b", 30}}));
        REQUIRE(combos[3] == json({{"a", 2}, {"b", 10}}));
    }

    SECTION("int, float and bool candidates keep their json types") {
        ParameterGrid grid;
        grid.addParameter("window", {10, 20});
        grid.addParameter("position_size_pct", {0.1, 0.2, 0.3});
        grid.addParameter("use_stop", {true, false});

        std::vector<json> combos = grid.generateCombinations();
        REQUIRE(combos.size() == 12);
        REQUIRE(combos[0]["window"].is_number_integer());
        REQUIRE(combos[0]["position_size_pct"].is_number_float());
        REQUIRE(combos[0]["use_stop"].is_boolean());
    }

    SECTION("no parameters means one combo: an empty object") {
        ParameterGrid grid;
        std::vector<json> combos = grid.generateCombinations();
        REQUIRE(combos.size() == 1);
        REQUIRE(combos[0].is_object());
        REQUIRE(combos[0].empty());
    }

    SECTION("a parameter with no candidates means no complete combo can exist") {
        ParameterGrid grid;
        grid.addParameter("a", {1, 2});
        grid.addParameter("b", {});
        REQUIRE(grid.generateCombinations().empty());
    }

    SECTION("parameterCount and parameterNames report parameters in the order added") {
        ParameterGrid grid;
        grid.addParameter("zeta", {1});
        grid.addParameter("alpha", {1});
        REQUIRE(grid.parameterCount() == 2);
        REQUIRE(grid.parameterNames() == std::vector<std::string>{"zeta", "alpha"});
    }
}

TEST_CASE("objectiveValue, objectiveName and parseObjective agree with each other", "[optimizer]") {
    OptimizationMetrics metrics;
    metrics.totalReturn = 12.5;
    metrics.cagr = 3.25;
    metrics.sharpeRatio = 1.75;

    SECTION("objectiveValue reads the field each objective names") {
        REQUIRE(objectiveValue(metrics, OptimizationObjective::TotalReturn) == Catch::Approx(12.5));
        REQUIRE(objectiveValue(metrics, OptimizationObjective::CAGR) == Catch::Approx(3.25));
        REQUIRE(objectiveValue(metrics, OptimizationObjective::Sharpe) == Catch::Approx(1.75));
    }

    SECTION("every objective's name parses back to itself") {
        for(OptimizationObjective objective : {OptimizationObjective::TotalReturn, OptimizationObjective::Sharpe, OptimizationObjective::CAGR}){
            OptimizationObjective parsed = OptimizationObjective::TotalReturn;
            //start from a different value than the one being parsed
            //where possible, so a parse that silently did nothing couldn't pass
            if(objective == OptimizationObjective::TotalReturn){
                parsed = OptimizationObjective::CAGR;
            }
            REQUIRE(parseObjective(objectiveName(objective), parsed));
            REQUIRE(parsed == objective);
        }
    }

    SECTION("an unrecognized name returns false and leaves the output untouched") {
        OptimizationObjective objective = OptimizationObjective::CAGR;
        REQUIRE_FALSE(parseObjective("profit", objective));
        REQUIRE_FALSE(parseObjective("", objective));
        REQUIRE_FALSE(parseObjective("Sharpe", objective)); //names are case-sensitive
        REQUIRE(objective == OptimizationObjective::CAGR);
    }
}

TEST_CASE("rankResults sorts best-first by the chosen objective", "[optimizer]") {
    //each result is best at a DIFFERENT objective, so ranking by each one
    //has to give a different order
    std::vector<OptimizationResult> results = {
        makeResult({{"id", "A"}}, 10.0, 1.0, 0.5),
        makeResult({{"id", "B"}}, 30.0, 2.0, 0.1),
        makeResult({{"id", "C"}}, 20.0, 3.0, 0.9),
    };

    auto idsOf = [](const std::vector<OptimizationResult>& ranked){
        std::string ids;
        for(const OptimizationResult& result : ranked){
            ids += result.parameters["id"].get<std::string>();
        }
        return ids;
    };

    REQUIRE(idsOf(rankResults(results, OptimizationObjective::TotalReturn)) == "BCA");
    REQUIRE(idsOf(rankResults(results, OptimizationObjective::CAGR)) == "CBA");
    REQUIRE(idsOf(rankResults(results, OptimizationObjective::Sharpe)) == "CAB");

    SECTION("the input vector is left in its original order") {
        rankResults(results, OptimizationObjective::TotalReturn);
        REQUIRE(idsOf(results) == "ABC");
    }

    SECTION("ties keep their original relative order") {
        std::vector<OptimizationResult> tied = {
            makeResult({{"id", "X"}}, 5.0, 0.0, 0.0),
            makeResult({{"id", "Y"}}, 9.0, 0.0, 0.0),
            makeResult({{"id", "Z"}}, 5.0, 0.0, 0.0),
            makeResult({{"id", "W"}}, 5.0, 0.0, 0.0),
        };
        REQUIRE(idsOf(rankResults(tied, OptimizationObjective::TotalReturn)) == "YXZW");
    }

    SECTION("an empty input ranks to an empty output") {
        REQUIRE(rankResults({}, OptimizationObjective::Sharpe).empty());
    }
}

TEST_CASE("exportOptimizationCSV writes a header and one ranked row per result", "[optimizer]") {
    TempFileGuard guard{tempFilePath("results.csv")};

    std::vector<OptimizationResult> ranked = {
        makeResult({{"fast_period", 5}, {"slow_period", 30}}, 25.5, 4.0, 1.5),
        makeResult({{"fast_period", 10}, {"slow_period", 50}}, -3.0, -0.5, -0.2),
    };

    exportOptimizationCSV(guard.path, {"fast_period", "slow_period"}, ranked);

    std::vector<std::string> lines = readLines(guard.path);
    REQUIRE(lines.size() == 3);
    REQUIRE(lines[0] == "Rank,fast_period,slow_period,TotalReturn,CAGR,SharpeRatio,MaxDrawdown,NumTrades");
    REQUIRE(lines[1] == "1,5,30,25.5,4,1.5,-10,5");
    REQUIRE(lines[2] == "2,10,50,-3,-0.5,-0.2,-10,5");

    SECTION("a string parameter containing a comma is quoted, and a missing parameter is an empty cell") {
        TempFileGuard guard2{tempFilePath("escaped.csv")};
        std::vector<OptimizationResult> odd = {
            makeResult({{"label", "a,b"}}, 1.0, 1.0, 1.0),
        };

        exportOptimizationCSV(guard2.path, {"label", "absent"}, odd);

        std::vector<std::string> oddLines = readLines(guard2.path);
        REQUIRE(oddLines.size() == 2);
        REQUIRE(oddLines[1] == "1,\"a,b\",,1,1,1,-10,5");
    }

    SECTION("metrics keep more than 6 significant digits, matching the json outputs") {
        TempFileGuard guard4{tempFilePath("precision.csv")};
        std::vector<OptimizationResult> precise = {
            makeResult({{"window", 20}}, 324.7362123, 15.56117456, 0.9205251234),
        };

        exportOptimizationCSV(guard4.path, {"window"}, precise);

        std::vector<std::string> preciseLines = readLines(guard4.path);
        REQUIRE(preciseLines.size() == 2);
        REQUIRE(preciseLines[1] == "1,20,324.7362123,15.56117456,0.9205251234,-10,5");
    }

    SECTION("no results still writes the header row") {
        TempFileGuard guard3{tempFilePath("empty.csv")};
        exportOptimizationCSV(guard3.path, {"window"}, {});

        std::vector<std::string> emptyLines = readLines(guard3.path);
        REQUIRE(emptyLines.size() == 1);
        REQUIRE(emptyLines[0] == "Rank,window,TotalReturn,CAGR,SharpeRatio,MaxDrawdown,NumTrades");
    }
}

TEST_CASE("exportOptimizationSummaryJSON names the best combo", "[optimizer]") {
    TempFileGuard guard{tempFilePath("summary.json")};

    OptimizerSetup setup;
    setup.simID = "my_sim";
    setup.strategyType = "sma";

    SECTION("the first ranked result is reported as best") {
        std::vector<OptimizationResult> ranked = {
            makeResult({{"fast_period", 5}}, 25.5, 4.0, 1.5),
            makeResult({{"fast_period", 10}}, -3.0, -0.5, -0.2),
        };

        exportOptimizationSummaryJSON(guard.path, setup, OptimizationObjective::Sharpe, ranked);

        std::ifstream file(guard.path);
        json summary = json::parse(file);
        REQUIRE(summary["simID"] == "my_sim");
        REQUIRE(summary["strategy"] == "sma");
        REQUIRE(summary["objective"] == "sharpe");
        REQUIRE(summary["combinationsTested"] == 2);
        REQUIRE(summary["best"]["parameters"] == json({{"fast_period", 5}}));
        REQUIRE(summary["best"]["objectiveValue"].get<double>() == Catch::Approx(1.5));
        REQUIRE(summary["best"]["totalReturn"].get<double>() == Catch::Approx(25.5));
        REQUIRE(summary["best"]["numTrades"] == 5);
    }

    SECTION("with no results, best is null rather than missing") {
        exportOptimizationSummaryJSON(guard.path, setup, OptimizationObjective::CAGR, {});

        std::ifstream file(guard.path);
        json summary = json::parse(file);
        REQUIRE(summary.contains("best"));
        REQUIRE(summary["best"].is_null());
        REQUIRE(summary["combinationsTested"] == 0);
        REQUIRE(summary["objective"] == "cagr");
    }
}

TEST_CASE("runOptimization runs one full simulation per combo", "[optimizer]") {
    std::unordered_map<std::string, Data> feeds;
    feeds.emplace("WAVY", makeWavyFeed("WAVY", "WAVY", 140));

    OptimizerSetup setup = makeDonSetup();

    ParameterGrid grid;
    grid.addParameter("window", {5, 10, 20});
    std::vector<json> combos = grid.generateCombinations();
    REQUIRE(combos.size() == 3);

    std::vector<OptimizationResult> results = runOptimization(setup, feeds, combos);

    SECTION("one result per combo, in the same order, carrying that combo's parameters") {
        REQUIRE(results.size() == 3);
        for(size_t i = 0; i < combos.size(); i++){
            REQUIRE(results[i].parameters == combos[i]);
        }
    }

    SECTION("different parameter values give 3 distinct, plausible result rows") {
        std::set<std::string> distinct;
        for(const OptimizationResult& result : results){
            //plausible: the strategy actually traded, and the numbers are
            //real finite values rather than nan/inf from a broken run
            REQUIRE(result.metrics.numTrades > 0);
            REQUIRE(std::isfinite(result.metrics.totalReturn));
            REQUIRE(std::isfinite(result.metrics.cagr));
            REQUIRE(std::isfinite(result.metrics.sharpeRatio));
            REQUIRE(result.metrics.maxDrawdown <= 0.0);

            //distinct: the window parameter genuinely changed what the
            //strategy did, rather than being silently ignored
            distinct.insert(json{result.metrics.totalReturn, result.metrics.numTrades}.dump());
        }
        REQUIRE(distinct.size() == 3);
    }

    SECTION("running the same combo again gives identical metrics - no state leaks between runs") {
        std::vector<OptimizationResult> again = runOptimization(setup, feeds, {combos[1], combos[0], combos[1]});
        REQUIRE(again.size() == 3);

        REQUIRE(again[0].metrics.totalReturn == Catch::Approx(results[1].metrics.totalReturn));
        REQUIRE(again[0].metrics.numTrades == results[1].metrics.numTrades);
        REQUIRE(again[1].metrics.totalReturn == Catch::Approx(results[0].metrics.totalReturn));
        REQUIRE(again[2].metrics.totalReturn == Catch::Approx(results[1].metrics.totalReturn));
        REQUIRE(again[2].metrics.sharpeRatio == Catch::Approx(results[1].metrics.sharpeRatio));
    }

    SECTION("the caller's feeds are left exactly as they were") {
        REQUIRE(feeds.at("WAVY").totalBars() == 140);
        REQUIRE(feeds.at("WAVY").hasMoreData());
        REQUIRE(feeds.at("WAVY").getBar().date == "2024-01-01");
    }

    SECTION("ranking the results by total return puts the best first") {
        std::vector<OptimizationResult> ranked = rankResults(results, OptimizationObjective::TotalReturn);
        REQUIRE(ranked.size() == 3);
        REQUIRE(ranked[0].metrics.totalReturn >= ranked[1].metrics.totalReturn);
        REQUIRE(ranked[1].metrics.totalReturn >= ranked[2].metrics.totalReturn);
    }
}

TEST_CASE("runOptimization handles degenerate inputs without crashing", "[optimizer]") {
    std::unordered_map<std::string, Data> feeds;
    feeds.emplace("WAVY", makeWavyFeed("WAVY", "WAVY", 60));

    SECTION("no feed ids means no results") {
        OptimizerSetup setup = makeDonSetup();
        setup.feedIDs.clear();
        REQUIRE(runOptimization(setup, feeds, {json({{"window", 5}})}).empty());
    }

    SECTION("no combos means no results") {
        REQUIRE(runOptimization(makeDonSetup(), feeds, {}).empty());
    }
}
