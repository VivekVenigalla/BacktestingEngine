#include "optimizer.hpp"
#include "account.hpp"
#include "broker.hpp"
#include "strategy.hpp"
#include "createStrat.hpp"
#include "performanceEval.hpp"
#include "simulationRunner.hpp"
#include "logger.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>

void ParameterGrid::addParameter(const std::string& name, const std::vector<nlohmann::json>& candidates){
    names.push_back(name);
    candidateLists.push_back(candidates);
}

std::vector<nlohmann::json> ParameterGrid::generateCombinations() const{
    std::vector<nlohmann::json> combos;

    //no parameters at all - "sweep nothing" still means "run once", with
    //whatever defaults the strategy itself falls back to(see
    //createStrat.cpp's config.value("position_size_pct", 0.2) pattern)
    if(names.empty()){
        combos.push_back(nlohmann::json::object());
        return combos;
    }

    //any parameter with zero candidates leaves that slot unfillable, so no
    //complete combination can exist at all - bail out with an empty
    //result rather than silently skipping that parameter
    for(const std::vector<nlohmann::json>& candidates : candidateLists){
        if(candidates.empty()){
            return combos;
        }
    }

    //indices[i] tracks which candidate of parameter i the CURRENT combo
    //uses - this is a classic "odometer"/mixed-radix counter, the same
    //idea as a car's odometer where each wheel can have a different number
    //of positions(here, each parameter's own candidate count). advancing
    //it means incrementing the rightmost wheel, and whenever a wheel wraps
    //back to 0 the carry moves one wheel to the left - repeating this
    //visits every combination exactly once, in the same order nested
    //for-loops(one per parameter, innermost = last parameter) would
    std::vector<size_t> indices(names.size(), 0);

    while(true){
        nlohmann::json combo;
        for(size_t i = 0; i < names.size(); i++){
            combo[names[i]] = candidateLists[i][indices[i]];
        }
        combos.push_back(combo);

        //advance the odometer, starting from the LAST parameter(the
        //fastest-moving "digit", exactly like nested for-loops'
        //innermost loop advancing every iteration while outer loops only
        //advance on a carry)
        int pos = static_cast<int>(names.size()) - 1;
        while(pos >= 0){
            indices[pos]++;
            if(indices[pos] < candidateLists[pos].size()){
                break; //no carry needed - this is the next combo to emit
            }
            indices[pos] = 0; //this digit wrapped around - carry into the previous one
            pos--;
        }
        //pos < 0 means every digit just carried out simultaneously - the
        //odometer has wrapped all the way back to all-zeros, meaning
        //we've already emitted every combination there is
        if(pos < 0){
            break;
        }
    }

    return combos;
}

double objectiveValue(const OptimizationMetrics& metrics, OptimizationObjective objective){
    switch(objective){
        case OptimizationObjective::TotalReturn:
            return metrics.totalReturn;
        case OptimizationObjective::Sharpe:
            return metrics.sharpeRatio;
        case OptimizationObjective::CAGR:
            return metrics.cagr;
    }
    //every enumerator is handled above - this is unreachable in practice,
    //but keeps the function well-defined if OptimizationObjective ever
    //grows a new enumerator this switch forgets to update
    throw std::invalid_argument("objectiveValue: unknown OptimizationObjective");
}

//runs one combo's parameters once, over a FRESH independent copy of every
//feed's full data, and returns the metrics that run produced. this is
//walkForward.cpp's runSlice, minus the date-range narrowing(every combo
//here trades the ENTIRE dataset, not one window of it) and with the
//strategy parameters coming from the combo instead of a fixed setup
static OptimizationMetrics runCombo(
    const OptimizerSetup& setup,
    const std::unordered_map<std::string, Data>& feeds,
    const nlohmann::json& comboParams
){
    //1. an independent, full copy of every feed - reusing sliceBars over
    //each feed's own first/last date is just a convenient way to get a
    //fresh copy with its own streaming position, the same trick runSlice
    //uses for one window; here the "window" is simply the whole feed
    std::unordered_map<std::string, Data> runFeeds;
    for(const std::string& id : setup.feedIDs){
        const Data& source = feeds.at(id);
        std::vector<std::string> dates = source.allDates();
        //an empty feed has nothing to trade at all - not expected in
        //practice(generateCombinations() never hands back an incomplete
        //combo), but guarded rather than indexing into an empty vector
        if(dates.empty()){
            return OptimizationMetrics{};
        }
        std::map<std::string, Bar> allBars = source.sliceBars(dates.front(), dates.back());
        runFeeds.try_emplace(id, source.ID, source.ticker, std::move(allBars));
    }

    //2. the shared bar maps main.cpp seeds with each feed's first bar
    //before building the Broker and Strategy
    std::unordered_map<std::string, Bar> bars;
    std::unordered_map<std::string, Bar> tempBars;
    for(const std::string& id : setup.feedIDs){
        tempBars[id] = runFeeds.at(id).getBar();
        bars[id] = tempBars[id];
    }

    //3. a fresh account/broker/metrics/logger, so every combo starts from
    //the initial balance with an empty trade history
    Account account(setup.initialBalance, setup.allTickers, "optimizerAccount");
    Broker broker(account, bars, setup.commissionRate, setup.slippageRate, "optimizerBroker");
    std::unordered_map<long int, Trade>& historyRef = broker.returnHistory();
    Metrics calculator(account, historyRef, setup.allTickers);
    Logger logger;

    //4. a fresh strategy, built with THIS combo's parameters - the whole
    //point of grid search is that each combo gets different parameter
    //values here, unlike walk-forward's runSlice which reuses the same
    //parameters across every window
    std::unique_ptr<Strategy> strategy = StrategyFactory::create(
        broker, account, tempBars, historyRef, setup.allTickers, setup.strategyType, comboParams
    );
    strategy->init();

    //5. drive it to completion, silently(no console spam, no export files -
    //a full grid sweep can mean dozens or hundreds of combos, so per-combo
    //console/file output would overwhelm both)
    std::unordered_map<std::string, double> currPrices;
    double initBalance = setup.initialBalance;
    size_t totalBars = runFeeds.at(setup.feedIDs.front()).totalBars();

    SimulationRunner runner(
        setup.simID, account, broker, strategy, logger, calculator,
        runFeeds, bars, tempBars, currPrices, setup.feedIDs, initBalance,
        setup.cagrLength, totalBars, ""
    );
    runner.setSilentMode(true);
    runner.runAll();

    //6. read the numbers straight off this run's own Logger/Metrics - the
    //same calls Logger::exportJSON makes, minus writing them to a file
    OptimizationMetrics metrics;
    metrics.totalReturn = calculator.totalReturn(initBalance, currPrices);
    metrics.cagr = calculator.cagr(initBalance, currPrices, static_cast<int>(setup.cagrLength));
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

std::vector<OptimizationResult> runOptimization(
    const OptimizerSetup& setup,
    const std::unordered_map<std::string, Data>& feeds,
    const std::vector<nlohmann::json>& combos
){
    std::vector<OptimizationResult> results;
    results.reserve(combos.size());

    if(setup.feedIDs.empty()){
        return results;
    }

    for(const nlohmann::json& combo : combos){
        OptimizationResult result;
        result.parameters = combo;
        result.metrics = runCombo(setup, feeds, combo);
        results.push_back(result);
    }

    return results;
}
