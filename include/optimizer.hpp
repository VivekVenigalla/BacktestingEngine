#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "nlohmann/json.hpp"
#include "dataFeed.hpp"

//parameter optimization(a.k.a "grid search") answers yet another different
//question than Monte Carlo or walk-forward: those two both take a
//strategy's parameters as GIVEN and ask "how much did luck/timing shape
//this result" and "does this still work on data it wasn't tuned on" - grid
//search instead asks "which parameter values work best in the first
//place", by actually trying a whole grid of candidate combinations and
//comparing their results
//
//ParameterGrid's only job is the combinatorics: turning a strategy's
//tunable parameters - each one paired with a list of candidate values to
//try - into the full CARTESIAN PRODUCT of every possible combination, the
//same set of combinations a nested for-loop(one loop per parameter) would
//visit, just generated programmatically instead of hand-written per
//strategy shape. it never runs a simulation itself - Day 12's driver is
//what actually feeds each combination into StrategyFactory::create() and
//runs it
//
//each parameter's candidates are stored as nlohmann::json values rather
//than one fixed C++ type, since different parameters on the SAME strategy
//can be different types(e.g smaCross's fast_period/slow_period are ints,
//position_size_pct is a float) - json already represents that naturally,
//and every combination this class produces is itself a json object shaped
//exactly like a batch config's "parameters" block(see createStrat.cpp),
//ready to hand straight to StrategyFactory::create() with no translation
class ParameterGrid{
    public:
        //adds one parameter to the grid, with the list of candidate
        //values to sweep over for it. parameters accumulate across calls -
        //call this once per tunable parameter before generateCombinations()
        void addParameter(const std::string& name, const std::vector<nlohmann::json>& candidates);

        //the full Cartesian product: one json object per combination, each
        //mapping every parameter name added so far to one of its candidate
        //values. size() == the PRODUCT of every parameter's candidate
        //count(e.g 3 candidates x 3 candidates = 9 combos, see Day 14's
        //test) - this is what makes grid search expensive as more
        //parameters/candidates are added, unlike Monte Carlo's fixed
        //numRuns
        //
        //special cases: no parameters added at all returns a single
        //combo(an empty json object) rather than zero, since "nothing to
        //sweep" still means "run once" - but a parameter added with ZERO
        //candidates returns an EMPTY vector(no combos at all), since
        //there's no valid value to fill that parameter's slot with, so no
        //complete combination can exist
        std::vector<nlohmann::json> generateCombinations() const;

        //how many distinct parameters have been added so far - mostly
        //useful for tests/callers sanity-checking the grid shape before
        //generating
        size_t parameterCount() const { return names.size(); }

    private:
        //names[i] and candidateLists[i] are kept as two parallel
        //vectors(rather than one combined struct) purely so
        //generateCombinations()'s odometer loop below can index them
        //together by position - names preserves insertion order, which is
        //also the order generateCombinations() enumerates combos in
        std::vector<std::string> names;
        std::vector<std::vector<nlohmann::json>> candidateLists;
};

//--- the driver: actually running a strategy once per combination ---
//everything above only computes WHICH parameter combinations exist. the
//pieces below run the strategy once per combo(over the FULL historical
//range - unlike walk-forward, grid search isn't about in/out-of-sample
//windows, it's about which values work best overall) and record what
//happened, so they can be compared and ranked

//the headline numbers worth comparing across combos. same definitions/
//units as metricData.json and WalkForwardMetrics: totalReturn/cagr/
//maxDrawdown are percentages, sharpeRatio is annualized
struct OptimizationMetrics{
    double totalReturn = 0.0;
    double cagr = 0.0;
    double sharpeRatio = 0.0;
    double maxDrawdown = 0.0;
    int numTrades = 0;
};

//which of OptimizationMetrics' fields a caller wants combos RANKED by -
//grid search produces every metric for every combo regardless(they're all
//computed for free from the same run), this just picks which single
//number "best" means when comparing across combos
enum class OptimizationObjective{
    TotalReturn,
    Sharpe,
    CAGR
};

//reads whichever field `objective` names out of a single combo's metrics -
//the one place that mapping lives, so Day 13's ranking/export logic(and
//anything else that needs to compare combos) doesn't have to duplicate a
//switch over OptimizationObjective itself
double objectiveValue(const OptimizationMetrics& metrics, OptimizationObjective objective);

//one combo's outcome: the exact parameter values that were run, plus the
//metrics that run produced
struct OptimizationResult{
    nlohmann::json parameters;
    OptimizationMetrics metrics;
};

//everything needed to build a FRESH Account/Broker/Strategy/Metrics/Logger
//per combo - the same "construct everything new every time" pattern
//main.cpp uses per simulation(see WalkForwardSetup's identical comment),
//reused here unchanged since SimulationRunner is still one-shot either way
struct OptimizerSetup{
    std::string simID;
    std::string strategyType;
    double initialBalance = 0.0;
    double commissionRate = 0.0;
    double slippageRate = 0.0;
    double cagrLength = 1.0;
    //the feeds this sim trades - every combo runs over ALL of these feeds'
    //full data(no slicing), the same feeds every combo shares as input
    std::vector<std::string> feedIDs;
    //every feed id in the whole batch config, in the same role main.cpp's
    //tempTickers plays for Account/Metrics/StrategyFactory
    std::vector<std::string> allTickers;
};

//runs the setup's strategy once per combo in `combos`(each combo becomes
//that run's "parameters" json, handed straight to StrategyFactory::create)
//over the full data in `feeds`, and collects every combo's metrics into an
//in-memory results table - one OptimizationResult per combo, in the SAME
//order `combos` was given in. feeds is only read from(each run gets its
//own independent copy via Data::sliceBars over the whole date range), so
//the caller's feeds are left exactly as they were
//
//unlike runWalkForward, nothing here is ever skipped: generateCombinations()
//already guarantees every combo is complete, and a combo running over the
//full dataset can't land on an empty date range the way a narrow
//walk-forward window sometimes can
std::vector<OptimizationResult> runOptimization(
    const OptimizerSetup& setup,
    const std::unordered_map<std::string, Data>& feeds,
    const std::vector<nlohmann::json>& combos
);
