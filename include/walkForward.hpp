#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include "dataFeed.hpp"
#include "nlohmann/json.hpp"

//walk-forward analysis answers a different question than Monte Carlo does:
//Monte Carlo resamples ONE completed run's own returns to see how much luck
//shaped it, but it never proves the strategy's PARAMETERS would still work
//on price data they weren't tuned against. walk-forward does that by
//rolling a window across history - tune/evaluate on an "in-sample" stretch,
//then immediately re-check the SAME parameters on the very next, unseen
//"out-of-sample" stretch, then slide forward and repeat. a strategy whose
//out-of-sample results consistently look much worse than its in-sample
//results is a classic sign of overfitting to one particular stretch of
//history, rather than having found a real, repeatable edge
struct WalkForwardWindow{
    //all four are date strings(e.g "2024-03-15"), matching the same
    //YYYY-MM-DD keys Data's internal std::map<std::string, Bar> is already
    //sorted by - both ranges are INCLUSIVE on both ends
    std::string inSampleStart = "";
    std::string inSampleEnd = "";
    std::string outSampleStart = "";
    std::string outSampleEnd = "";
};

//sortedDates: every available bar date, in chronological order(a plain
//std::vector<std::string> - Day 7 is what actually slices Data's bars by
//the date bounds this produces; this function only computes WHERE the
//bounds fall, it never touches Data or Bar itself)
//
//inSampleBars/outSampleBars: how many bars each window's two halves span
//
//stepBars: how far the START of the window advances between one window and
//the next - a stepBars smaller than inSampleBars means consecutive windows'
//in-sample ranges overlap(a denser, more expensive sweep); stepBars equal
//to inSampleBars means they never overlap
//
//returns every window that fully fits within sortedDates - a window
//that would run off the end of the available dates simply isn't included,
//rather than being generated with missing/truncated data
std::vector<WalkForwardWindow> generateWindows(
    const std::vector<std::string>& sortedDates,
    int inSampleBars,
    int outSampleBars,
    int stepBars
);

//--- the driver: actually running a strategy over each window ---
//everything above only computes WHERE the windows fall. the pieces below
//run the strategy over them and record what happened

//the handful of numbers worth comparing between an in-sample run and its
//out-of-sample twin. same definitions/units as metricData.json:
//totalReturn and maxDrawdown are percentages, sharpeRatio is annualized
struct WalkForwardMetrics{
    double totalReturn = 0.0;
    double sharpeRatio = 0.0;
    double maxDrawdown = 0.0;
    int numTrades = 0;
};

//one window's outcome: the same strategy parameters, run once over the
//in-sample stretch and once over the out-of-sample stretch that follows it
struct WalkForwardResult{
    WalkForwardWindow window;
    WalkForwardMetrics inSample;
    WalkForwardMetrics outSample;
};

//everything needed to build a FRESH Account/Broker/Strategy/Metrics/Logger
//per run - the same "construct everything new every time" pattern main.cpp
//uses for each simulation, since SimulationRunner is one-shot and has no
//reset(). main.cpp fills this in from the sim's own config entry
struct WalkForwardSetup{
    std::string simID;
    std::string strategyType;
    nlohmann::json strategyParams = nlohmann::json::object();
    double initialBalance = 0.0;
    double commissionRate = 0.0;
    double slippageRate = 0.0;
    double cagrLength = 1.0;
    //the feeds this sim trades - the FIRST one is the "primary" feed,
    //whose dates decide where the windows fall(exactly like SimulationRunner
    //treats feedIDs[0]); every feed is sliced by the same date bounds
    std::vector<std::string> feedIDs;
    //every feed id in the whole batch config, in the same role main.cpp's
    //tempTickers plays for Account/Metrics/StrategyFactory
    std::vector<std::string> allTickers;
};

//for every window generateWindows() produces over the primary feed's dates,
//runs the strategy over the in-sample slice, then over the out-of-sample
//slice, and collects both sets of metrics. feeds is only read from(each
//slice is a COPY made by Data::sliceBars), so the caller's feeds are left
//exactly as they were. a window is skipped(with a message on stderr) if
//any feed has no bars at all inside one of its ranges
//
//known simplification: every run starts from a cold start - an indicator
//needing N bars of history(e.g a 50-bar slow SMA) can't trade until N bars
//into EACH slice, so windows should be comfortably longer than the
//strategy's longest lookback
std::vector<WalkForwardResult> runWalkForward(
    const WalkForwardSetup& setup,
    const std::unordered_map<std::string, Data>& feeds,
    int inSampleBars,
    int outSampleBars,
    int stepBars
);

//writes results as walkForwardResults.json at filepath - one entry per
//window, each with its in-sample and out-of-sample metrics side by side
void exportWalkForwardJSON(
    const std::filesystem::path& filepath,
    const WalkForwardSetup& setup,
    int inSampleBars,
    int outSampleBars,
    int stepBars,
    const std::vector<WalkForwardResult>& results
);
