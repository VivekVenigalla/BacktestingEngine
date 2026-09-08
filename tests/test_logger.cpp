#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <fstream>
#include <filesystem>
#include "logger.hpp"
#include "projectPaths.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

// Small RAII helper: removes whatever path it holds when it goes out of
// scope, even if a REQUIRE fails partway through the test (Catch2 unwinds
// the stack via a normal C++ exception on failure, so this destructor still
// runs). This keeps filesystem-touching tests from littering behind them,
// win or lose.
struct TempPathGuard {
    fs::path path;
    ~TempPathGuard() {
        std::error_code ec;
        fs::remove_all(path, ec); // ignore errors; nothing to clean up if it was never created
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
    acct.modifyBalance(500.0); // equity now 10500 -> 5% total return / CAGR over 1 year

    Trade filled1, filled2, unfilled;
    filled1.filled = true;
    filled2.filled = true;
    unfilled.filled = false;
    std::unordered_map<long int, Trade> history{{1, filled1}, {2, filled2}, {3, unfilled}};

    Metrics calc(acct, history, {});
    std::unordered_map<std::string, double> prices; // cash-only equity
    double initBalance = 10000.0;
    double cagrLength = 1.0;
    std::string simID = "testSim";

    fs::path outPath = fs::temp_directory_path() / "backtest_engine_test_metricData.json";
    TempPathGuard guard{outPath};

    logger.exportJSON(outPath, "metricData.json", calc, simID, prices, initBalance, cagrLength, history);

    std::ifstream file(outPath);
    REQUIRE(file.is_open());
    json parsed;
    file >> parsed;

    REQUIRE(parsed["simID"] == "testSim");
    REQUIRE(parsed["totalReturns"].get<double>() == Catch::Approx(5.0));
    REQUIRE(parsed["CAGR"].get<double>() == Catch::Approx(5.0));
    REQUIRE(parsed["Trade_records"]["Number_of_trades"] == 3);
    REQUIRE(parsed["Trade_records"]["Successful_trades"] == 2);
    REQUIRE(parsed["Trade_records"]["Unsuccessful_trades"] == 1);
}

TEST_CASE("exportData creates a fresh simulation folder with all three output files", "[logger]") {
    // exportData's output location isn't a parameter -- it's always
    // PROJECT_SOURCE_DIR/output/<batchID>/<simID>, so (unlike the other
    // export* tests above) we can't point it at a scratch temp directory.
    // We use a batch id unlikely to collide with anything real, and clean
    // it up both before AND after: before, in case a previous crashed test
    // run left it behind (exportData would otherwise pick a "_1" suffixed
    // folder instead of the exact name this test expects); after, via the
    // guard, so a normal run doesn't leave anything behind either.
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
