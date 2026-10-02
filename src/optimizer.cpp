#include "optimizer.hpp"
#include "account.hpp"
#include "broker.hpp"
#include "strategy.hpp"
#include "createStrat.hpp"
#include "performanceEval.hpp"
#include "simulationRunner.hpp"
#include "logger.hpp"
#include <algorithm>
#include <fstream>
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

std::string objectiveName(OptimizationObjective objective){
    switch(objective){
        case OptimizationObjective::TotalReturn:
            return "total_return";
        case OptimizationObjective::Sharpe:
            return "sharpe";
        case OptimizationObjective::CAGR:
            return "cagr";
    }
    throw std::invalid_argument("objectiveName: unknown OptimizationObjective");
}

bool parseObjective(const std::string& name, OptimizationObjective& objective){
    if(name == "total_return"){
        objective = OptimizationObjective::TotalReturn;
        return true;
    }
    if(name == "sharpe"){
        objective = OptimizationObjective::Sharpe;
        return true;
    }
    if(name == "cagr"){
        objective = OptimizationObjective::CAGR;
        return true;
    }
    return false;
}

std::vector<OptimizationResult> rankResults(
    const std::vector<OptimizationResult>& results,
    OptimizationObjective objective
){
    std::vector<OptimizationResult> ranked = results;
    //std::stable_sort keeps equal elements in their original relative
    //order(plain std::sort makes no such promise). the comparator says
    //"a goes before b" when a's objective value is strictly HIGHER, which
    //puts the best combo first
    std::stable_sort(ranked.begin(), ranked.end(),
        [objective](const OptimizationResult& a, const OptimizationResult& b){
            return objectiveValue(a.metrics, objective) > objectiveValue(b.metrics, objective);
        }
    );
    return ranked;
}

//one parameter value as csv cell text: strings go in raw(a json string's
//dump() would add surrounding quotes), numbers/bools use json's own text,
//and any cell containing a comma, quote or newline is wrapped in quotes
//with inner quotes doubled - the standard csv escaping rule
static std::string csvCell(const nlohmann::json& value){
    std::string text = value.is_string() ? value.get<std::string>() : value.dump();
    if(text.find_first_of(",\"\n") == std::string::npos){
        return text;
    }
    std::string escaped = "\"";
    for(char c : text){
        if(c == '"'){
            escaped += "\"\"";
        }
        else{
            escaped += c;
        }
    }
    escaped += "\"";
    return escaped;
}

void exportOptimizationCSV(
    const std::filesystem::path& filepath,
    const std::vector<std::string>& parameterNames,
    const std::vector<OptimizationResult>& rankedResults
){
    std::ofstream file(filepath);

    if(!file){
        std::cerr << "File " << filepath.filename().string() << " unable to be created. Terminating export..." << std::endl;
        return;
    }

    file << "Rank";
    for(const std::string& name : parameterNames){
        file << "," << csvCell(name);
    }
    file << ",TotalReturn,CAGR,SharpeRatio,MaxDrawdown,NumTrades\n";

    for(size_t i = 0; i < rankedResults.size(); i++){
        const OptimizationResult& result = rankedResults[i];
        file << (i + 1);
        for(const std::string& name : parameterNames){
            file << ",";
            if(result.parameters.contains(name)){
                file << csvCell(result.parameters[name]);
            }
        }
        file << "," << result.metrics.totalReturn
             << "," << result.metrics.cagr
             << "," << result.metrics.sharpeRatio
             << "," << result.metrics.maxDrawdown
             << "," << result.metrics.numTrades << "\n";
    }

    file.close();

    std::cout << "CSV File " << filepath.filename().string() << " created..." << std::endl;
}

void exportOptimizationSummaryJSON(
    const std::filesystem::path& filepath,
    const OptimizerSetup& setup,
    OptimizationObjective objective,
    const std::vector<OptimizationResult>& rankedResults
){
    std::ofstream file(filepath);

    if(!file){
        std::cerr << "File " << filepath.filename().string() << " unable to be created. Terminating export..." << std::endl;
        return;
    }

    nlohmann::json summary;
    summary["simID"] = setup.simID;
    summary["strategy"] = setup.strategyType;
    summary["objective"] = objectiveName(objective);
    summary["combinationsTested"] = rankedResults.size();

    if(rankedResults.empty()){
        summary["best"] = nullptr;
    }
    else{
        const OptimizationResult& best = rankedResults.front();
        summary["best"]["parameters"] = best.parameters;
        summary["best"]["objectiveValue"] = objectiveValue(best.metrics, objective);
        summary["best"]["totalReturn"] = best.metrics.totalReturn;
        summary["best"]["cagr"] = best.metrics.cagr;
        summary["best"]["sharpeRatio"] = best.metrics.sharpeRatio;
        summary["best"]["maxDrawdown"] = best.metrics.maxDrawdown;
        summary["best"]["numTrades"] = best.metrics.numTrades;
    }

    file << summary.dump(4);
    file.close();

    std::cout << "JSON File " << filepath.filename().string() << " created..." << std::endl;
}
