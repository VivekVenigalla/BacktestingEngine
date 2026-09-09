#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <fstream>
#include <filesystem>
#include "logger.hpp"
#include "projectPaths.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

//Small RAII helper: removes whatever path it holds when it goes out of
//scope, even if a REQUIRE fails partway through the test (Catch2 unwinds
//the stack via a normal C++ exception on failure, so this destructor still
//runs). This keeps filesystem-touching tests from littering behind them,
//win or lose.
struct TempPathGuard {
    fs::path path;
    //"~TempPathGuard()" is the destructor - the function that automatically
    //runs the moment an object of this type is destroyed(here, when a
    //TempPathGuard local variable goes out of scope at the end of a
    //TEST_CASE). that automatic cleanup is exactly what RAII means: tie a
    //resource's lifetime to an object's lifetime, so cleanup can't be
    //forgotten
    ~TempPathGuard() {
        //fs::remove_all(path, ec) is the error-code overload - instead of
        //throwing an exception on failure(the default behavior), it writes
        //any error into ec and returns normally. used here because
        //throwing out of a destructor is dangerous/generally forbidden in
        //c++, so a destructor needs a way to attempt cleanup without risking
        //an exception escaping
        std::error_code ec;
        fs::remove_all(path, ec); //ignore errors; nothing to clean up if it was never created
    }
};

TEST_CASE("logSnapshot appends a correctly-valued entry to fullHistory", "[logger]") {
    Logger logger;

    Bar bar;
    bar.ticker = "AAPL";
    bar.date = "2024-01-01";
    bar.open = 100.0;
    bar.high = 105.0;
    bar.low = 95.0;
    bar.close = 102.0;
    bar.volume = 1000;

    Position pos;
    pos.ticker = "AAPL";
    pos.quantity = 10;
    pos.average_entry_price = 100.0;

    logger.logSnapshot("2024-01-01", {{"AAPL", bar}}, 9000.0, 10500.0, {{"AAPL", pos}}, 0.0);

    REQUIRE(logger.fullHistory.dates.size() == 1);
    REQUIRE(logger.fullHistory.dates[0] == "2024-01-01");
    REQUIRE(logger.fullHistory.balances[0] == Catch::Approx(9000.0));
    REQUIRE(logger.fullHistory.totalEquity[0] == Catch::Approx(10500.0));
    REQUIRE(logger.fullHistory.drawDown[0] == Catch::Approx(0.0));
    REQUIRE(logger.fullHistory.bars[0].at("AAPL").close == Catch::Approx(102.0));
    REQUIRE(logger.fullHistory.positions[0].at("AAPL").quantity == 10);
}

TEST_CASE("exportCSV writes the equity-curve header and one row per logged snapshot", "[logger]") {
    Logger logger;

    Bar bar;
    bar.ticker = "AAPL";
    bar.date = "2024-01-01";
    bar.open = 100.0;
    bar.high = 105.0;
    bar.low = 95.0;
    bar.close = 102.0;
    bar.volume = 1000;

    Position pos;
    pos.ticker = "AAPL";
    pos.quantity = 10;
    pos.average_entry_price = 100.0;

    logger.logSnapshot("2024-01-01", {{"AAPL", bar}}, 9000.0, 10500.0, {{"AAPL", pos}}, 0.0);

    //fs::temp_directory_path() finds the os's own scratch directory(e.g
    ///tmp on mac/linux) - writing test output there instead of anywhere in
    //the project keeps this test fully isolated from the real repo, and
    //TempPathGuard(declared right after) ensures it gets cleaned up
    fs::path outPath = fs::temp_directory_path() / "backtest_engine_test_dynamicData.csv";
    TempPathGuard guard{outPath};

    logger.exportCSV(outPath, "dynamicData.csv");

    std::ifstream file(outPath);
    REQUIRE(file.is_open());

    std::string header, row;
    std::getline(file, header);
    std::getline(file, row);

    REQUIRE(header == "Date,Balance,Equity,DrawDown,Ticker,BarOpen,BarHigh,BarLow,BarClose,BarVolume,Quantity,AEP");
    REQUIRE(row == "2024-01-01,9000,10500,0,AAPL,100,105,95,102,1000,10,100");
}

TEST_CASE("exportCSVTrade writes the trade-history header and one row per trade", "[logger]") {
    Logger logger;

    Trade trade;
    trade.ticker = "AAPL";
    trade.execPrice = 101.0;
    trade.type = "market";
    trade.side = 0;
    trade.quantity = 10;
    trade.checkPrice = -1.0;
    trade.commision = 1.0;
    trade.filled = true;
    trade.status = "FILLED";
    trade.currBalance = 9000.0;

    std::unordered_map<long int, Trade> history{{5, trade}};

    fs::path outPath = fs::temp_directory_path() / "backtest_engine_test_tradeData.csv";
    TempPathGuard guard{outPath};

    logger.exportCSVTrade(outPath, "tradeData.csv", history);

    std::ifstream file(outPath);
    REQUIRE(file.is_open());

    std::string header, row;
    std::getline(file, header);
    std::getline(file, row);

    REQUIRE(header == "TradeID,TickerID,ExecPrice,Type,Side,Quantity,CheckPrice,Commission,Filled,Status,CurrentBalance");
    REQUIRE(row == "5,AAPL,101,market,0,10,-1,1,true,FILLED,9000");
}

TEST_CASE("exportJSON writes correctly-computed metrics and trade counts", "[logger]") {
    Logger logger;

    Account acct(10000.0);
    acct.modifyBalance(500.0); //equity now 10500 -> 5% total return / CAGR over 1 year

    //filled1 is a winning sell, filled2 a losing sell, unfilled excluded from
    //win-rate/profit-factor entirely (side/realizedPnL set explicitly on all
    //three -- winRate()/profitFactor() read `side`, so leaving it
    //default/uninitialized here would be reading garbage memory).
    Trade filled1, filled2, unfilled;
    filled1.filled = true; filled1.side = 1; filled1.realizedPnL = 100.0;
    filled2.filled = true; filled2.side = 1; filled2.realizedPnL = -50.0;
    unfilled.filled = false; unfilled.side = 1; unfilled.realizedPnL = 0.0;
    std::unordered_map<long int, Trade> history{{1, filled1}, {2, filled2}, {3, unfilled}};

    Metrics calc(acct, history, {});
    std::unordered_map<std::string, double> prices; //cash-only equity
    double initBalance = 10000.0;
    double cagrLength = 1.0;
    std::string simID = "testSim";

    //Log a small equity curve so Sharpe/Sortino/MaxDrawdown/BenchmarkReturn
    //(which read fullHistory, not the scalar args above) have real data to
    //work with: equity rises 10000->10608 then dips to 10200.
    Bar barA; barA.ticker = "AAPL"; barA.date = "2024-01-01"; barA.open = 100.0; barA.high = 100.0; barA.low = 100.0; barA.close = 100.0; barA.volume = 1000;
    Bar barB; barB.ticker = "AAPL"; barB.date = "2024-01-02"; barB.open = 106.0; barB.high = 106.0; barB.low = 106.0; barB.close = 106.08; barB.volume = 1000;
    Bar barC; barC.ticker = "AAPL"; barC.date = "2024-01-03"; barC.open = 105.0; barC.high = 105.0; barC.low = 105.0; barC.close = 105.0; barC.volume = 1000;

    logger.logSnapshot("2024-01-01", {{"AAPL", barA}}, 10000.0, 10000.0, {}, calc.drawDown(10000.0));
    logger.logSnapshot("2024-01-02", {{"AAPL", barB}}, 10608.0, 10608.0, {}, calc.drawDown(10608.0));
    logger.logSnapshot("2024-01-03", {{"AAPL", barC}}, 10200.0, 10200.0, {}, calc.drawDown(10200.0));

    //Independently compute the expected values from the exact same inputs
    //exportJSON will use internally -- this checks the *wiring* (does
    //exportJSON correctly read fullHistory and pass it through Metrics),
    //since the formulas themselves are already verified against hand-picked
    //numbers in test_performance.cpp.
    std::vector<double> expectedReturns = Metrics::returnsFromEquityCurve({10000.0, 10608.0, 10200.0});
    double expectedSharpe = calc.sharpeRatio(expectedReturns, 0.0, 1.0);
    double expectedSortino = calc.sortinoRatio(expectedReturns, 0.0, 1.0);
    double expectedMaxDrawdown = calc.maxDrawdown();
    double expectedBenchmark = calc.benchmarkReturn(100.0, 105.0);

    fs::path outPath = fs::temp_directory_path() / "backtest_engine_test_metricData.json";
    TempPathGuard guard{outPath};

    logger.exportJSON(outPath, "metricData.json", calc, simID, prices, initBalance, cagrLength, history, 1.0);

    std::ifstream file(outPath);
    REQUIRE(file.is_open());
    json parsed;
    file >> parsed;

    REQUIRE(parsed["simID"] == "testSim");
    REQUIRE(parsed["totalReturns"].get<double>() == Catch::Approx(5.0));
    REQUIRE(parsed["CAGR"].get<double>() == Catch::Approx(5.0));
    REQUIRE(parsed["MaxDrawdown"].get<double>() == Catch::Approx(expectedMaxDrawdown));
    REQUIRE(parsed["SharpeRatio"].get<double>() == Catch::Approx(expectedSharpe));
    REQUIRE(parsed["SortinoRatio"].get<double>() == Catch::Approx(expectedSortino));
    REQUIRE(parsed["BenchmarkReturn"].get<double>() == Catch::Approx(expectedBenchmark));
    REQUIRE(parsed["Trade_records"]["Number_of_trades"] == 3);
    REQUIRE(parsed["Trade_records"]["Successful_trades"] == 2);
    REQUIRE(parsed["Trade_records"]["Unsuccessful_trades"] == 1);
    //1 win (filled1, +100) of 2 closed trades (filled1, filled2) -> 50%
    REQUIRE(parsed["Trade_records"]["Win_rate"].get<double>() == Catch::Approx(50.0));
    //grossProfit=100, grossLoss=50 -> 2.0
    REQUIRE(parsed["Trade_records"]["Profit_factor"].get<double>() == Catch::Approx(2.0));
}

TEST_CASE("exportData creates a fresh simulation folder with all three output files", "[logger]") {
    //exportData's output location isn't a parameter -- it's always
    //PROJECT_SOURCE_DIR/output/<batchID>/<simID>, so (unlike the other
    //export* tests above) we can't point it at a scratch temp directory.
    //We use a batch id unlikely to collide with anything real, and clean
    //it up both before AND after: before, in case a previous crashed test
    //run left it behind (exportData would otherwise pick a "_1" suffixed
    //folder instead of the exact name this test expects); after, via the
    //guard, so a normal run doesn't leave anything behind either.
    std::string batchID = "unit_test_logger_batch";
    std::string simID = "logger_test_sim";
    fs::path batchDir = fs::path(PROJECT_SOURCE_DIR) / "output" / batchID;

    std::error_code ec;
    fs::remove_all(batchDir, ec);
    TempPathGuard guard{batchDir};

    Logger logger;
    Bar bar;
    bar.ticker = "AAPL";
    bar.date = "2024-01-01";
    bar.open = 100.0;
    bar.high = 105.0;
    bar.low = 95.0;
    bar.close = 102.0;
    bar.volume = 1000;
    logger.logSnapshot("2024-01-01", {{"AAPL", bar}}, 10000.0, 10000.0, {}, 0.0);

    Account acct(10000.0);
    std::unordered_map<long int, Trade> history;
    Metrics calc(acct, history, {});
    std::unordered_map<std::string, double> prices;
    double initBalance = 10000.0;
    double cagrLength = 1.0;

    logger.exportData(simID, calc, history, prices, initBalance, cagrLength, batchID);

    fs::path simDir = batchDir / simID;
    REQUIRE(fs::exists(simDir / "dynamicData.csv"));
    REQUIRE(fs::exists(simDir / "tradeData.csv"));
    REQUIRE(fs::exists(simDir / "metricData.json"));
}
