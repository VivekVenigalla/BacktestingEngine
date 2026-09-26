#include "walkForward.hpp"
#include "account.hpp"
#include "broker.hpp"
#include "strategy.hpp"
#include "createStrat.hpp"
#include "performanceEval.hpp"
#include "simulationRunner.hpp"
#include "logger.hpp"
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>

std::vector<WalkForwardWindow> generateWindows(
    const std::vector<std::string>& sortedDates,
    int inSampleBars,
    int outSampleBars,
    int stepBars
){
    std::vector<WalkForwardWindow> windows;

    //nothing sane to generate from a non-positive size/step - rather than
    //looping forever(a stepBars of 0 would never advance) or indexing
    //negatively, just hand back an empty result
    if(inSampleBars <= 0 || outSampleBars <= 0 || stepBars <= 0){
        return windows;
    }

    size_t windowSpan = static_cast<size_t>(inSampleBars) + static_cast<size_t>(outSampleBars);
    if(sortedDates.size() < windowSpan){
        return windows;
    }

    //start is the index of the FIRST bar in this window's in-sample half -
    //every iteration below carves one window out of [start, start+windowSpan)
    //and then slides start forward by stepBars for the next one
    size_t start = 0;
    while(start + windowSpan <= sortedDates.size()){
        WalkForwardWindow window;

        size_t inEndIndex = start + static_cast<size_t>(inSampleBars) - 1;
        size_t outStartIndex = start + static_cast<size_t>(inSampleBars);
        size_t outEndIndex = start + windowSpan - 1;

        window.inSampleStart = sortedDates[start];
        window.inSampleEnd = sortedDates[inEndIndex];
        window.outSampleStart = sortedDates[outStartIndex];
        window.outSampleEnd = sortedDates[outEndIndex];

        windows.push_back(window);

        start += static_cast<size_t>(stepBars);
    }

    return windows;
}

//runs the setup's strategy once over just [startDate, endDate] of every feed
//and returns that run's metrics - or nothing at all if any feed has no bars
//in that range(see runWalkForward's header comment)
//
//this is main.cpp's per-simulation construction sequence, condensed: fresh
//Account -> Broker -> Metrics -> Strategy -> Logger -> SimulationRunner, all
//local to this function so nothing from one run can leak into the next
static std::optional<WalkForwardMetrics> runSlice(
    const WalkForwardSetup& setup,
    const std::unordered_map<std::string, Data>& feeds,
    const std::string& startDate,
    const std::string& endDate
){
    //1. slice every feed down to this range, each slice becoming its own
    //streamable Data. try_emplace constructs each Data directly inside the
    //map from these arguments, so it is never copied or moved afterward -
    //which matters, since a Data holds an iterator into its OWN bar map
    std::unordered_map<std::string, Data> sliceFeeds;
    for(const std::string& id : setup.feedIDs){
        const Data& source = feeds.at(id);
        std::map<std::string, Bar> slice = source.sliceBars(startDate, endDate);
        if(slice.empty()){
            return std::nullopt;
        }
        sliceFeeds.try_emplace(id, source.ID, source.ticker, std::move(slice));
    }

    //2. the shared bar maps main.cpp seeds with each feed's first bar
    //before building the Broker and Strategy
    std::unordered_map<std::string, Bar> bars;
    std::unordered_map<std::string, Bar> tempBars;
    for(const std::string& id : setup.feedIDs){
        tempBars[id] = sliceFeeds.at(id).getBar();
        bars[id] = tempBars[id];
    }

    //3. a fresh account/broker/metrics/logger, so every run starts from
    //the initial balance with an empty trade history
    Account account(setup.initialBalance, setup.allTickers, "walkForwardAccount");
    Broker broker(account, bars, setup.commissionRate, setup.slippageRate, "walkForwardBroker");
    std::unordered_map<long int, Trade>& historyRef = broker.returnHistory();
    Metrics calculator(account, historyRef, setup.allTickers);
    Logger logger;

    //4. a fresh strategy - same parameters every window, which is the
    //whole point: nothing is re-tuned between the in-sample and
    //out-of-sample runs
    std::unique_ptr<Strategy> strategy = StrategyFactory::create(
        broker, account, tempBars, historyRef, setup.allTickers, setup.strategyType, setup.strategyParams
    );
    strategy->init();

    //5. drive it to completion, silently(no console spam, no export files)
    std::unordered_map<std::string, double> currPrices;
    double initBalance = setup.initialBalance;
    size_t totalBars = sliceFeeds.at(setup.feedIDs.front()).totalBars();

    SimulationRunner runner(
        setup.simID, account, broker, strategy, logger, calculator,
        sliceFeeds, bars, tempBars, currPrices, setup.feedIDs, initBalance,
        setup.cagrLength, totalBars, ""
    );
    runner.setSilentMode(true);
    runner.runAll();

    //6. read the numbers straight off this run's own Logger/Metrics - the
    //same calls Logger::exportJSON makes, minus writing them to a file
    WalkForwardMetrics metrics;
    metrics.totalReturn = calculator.totalReturn(initBalance, currPrices);
    metrics.maxDrawdown = calculator.maxDrawdown();
    std::vector<double> returns = Metrics::returnsFromEquityCurve(logger.fullHistory.totalEquity);
    metrics.sharpeRatio = calculator.sharpeRatio(returns, 0.0, 252.0);
    for(const auto& [id, trade] : historyRef){
        if(trade.filled){
            metrics.numTrades++;
        }
    }
    return metrics;
}

std::vector<WalkForwardResult> runWalkForward(
    const WalkForwardSetup& setup,
    const std::unordered_map<std::string, Data>& feeds,
    int inSampleBars,
    int outSampleBars,
    int stepBars
){
    std::vector<WalkForwardResult> results;

    if(setup.feedIDs.empty()){
        return results;
    }

    //the primary feed's dates decide where every window falls
    std::vector<WalkForwardWindow> windows = generateWindows(
        feeds.at(setup.feedIDs.front()).allDates(), inSampleBars, outSampleBars, stepBars
    );

    for(size_t i = 0; i < windows.size(); i++){
        const WalkForwardWindow& window = windows[i];

        std::optional<WalkForwardMetrics> inSample = runSlice(setup, feeds, window.inSampleStart, window.inSampleEnd);
        std::optional<WalkForwardMetrics> outSample = runSlice(setup, feeds, window.outSampleStart, window.outSampleEnd);

        if(!inSample || !outSample){
            std::cerr << "Walk-forward window " << (i + 1) << " skipped: a feed has no bars in "
                      << window.inSampleStart << " to " << window.outSampleEnd << std::endl;
            continue;
        }

        WalkForwardResult result;
        result.window = window;
        result.inSample = *inSample;
        result.outSample = *outSample;
        results.push_back(result);
    }

    return results;
}

//one side(in-sample or out-of-sample) of one window's json entry
static nlohmann::json sideToJSON(const std::string& start, const std::string& end, const WalkForwardMetrics& metrics){
    nlohmann::json side;
    side["start"] = start;
    side["end"] = end;
    side["totalReturn"] = metrics.totalReturn;
    side["sharpeRatio"] = metrics.sharpeRatio;
    side["maxDrawdown"] = metrics.maxDrawdown;
    side["numTrades"] = metrics.numTrades;
    return side;
}

void exportWalkForwardJSON(
    const std::filesystem::path& filepath,
    const WalkForwardSetup& setup,
    int inSampleBars,
    int outSampleBars,
    int stepBars,
    const std::vector<WalkForwardResult>& results
){
    std::ofstream file(filepath);

    if(!file){
        std::cerr << "File " << filepath.filename().string() << " unable to be created. Terminating export..." << std::endl;
        return;
    }

    nlohmann::json output;
    output["simID"] = setup.simID;
    output["inSampleBars"] = inSampleBars;
    output["outSampleBars"] = outSampleBars;
    output["stepBars"] = stepBars;
    output["windows"] = nlohmann::json::array();

    for(size_t i = 0; i < results.size(); i++){
        const WalkForwardResult& result = results[i];
        nlohmann::json entry;
        //1-based, since this is the "window #" a person reads in a table
        entry["window"] = i + 1;
        entry["inSample"] = sideToJSON(result.window.inSampleStart, result.window.inSampleEnd, result.inSample);
        entry["outSample"] = sideToJSON(result.window.outSampleStart, result.window.outSampleEnd, result.outSample);
        output["windows"].push_back(entry);
    }

    file << output.dump(4);
    file.close();

    std::cout << "JSON File " << filepath.filename().string() << " created..." << std::endl;
}
