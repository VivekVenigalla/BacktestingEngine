#pragma once
#include "../include/csvParser.hpp"
#include "../include/account.hpp"
#include "../include/broker.hpp"
#include "../include/dataFeed.hpp"
#include "../include/strategy.hpp"
#include "../include/performanceEval.hpp"
#include "logger.hpp"
#include <memory>
#include "nlohmann/json.hpp"


using json = nlohmann::json;

//SimulationRunner is the "conductor" that ties Account/Broker/Strategy/
//Logger/Metrics together and drives one simulation forward bar by bar - see
//step() in the .cpp for the exact order things happen in each bar, which
//matters a lot for avoiding lookahead bias(using future information a real
//trader wouldn't have had yet)
class SimulationRunner {


	public:
		//most of these parameters are references(Account&, Broker&, ...)
		//rather than plain values - SimulationRunner doesn't own any of
		//these objects, it just gets to use the exact same ones main.cpp
		//already created, so nothing here needs to be copied or kept in
		//sync separately. std::unique_ptr<Strategy>& is a reference to a
		//smart pointer specifically(not a plain Strategy&), because the
		//runner needs to call methods THROUGH whatever concrete strategy
		//the unique_ptr owns, without taking ownership of it itself
		SimulationRunner(const std::string& id,
			Account& accountRef,
			Broker& brokerRef,
			std::unique_ptr<Strategy>& strategyRef,
			Logger& loggerRef,
			Metrics& metricsRef,
			std::unordered_map<std::string, Data>& sharedFeeds,
			std::unordered_map<std::string, Bar>& sharedBars,
			std::unordered_map<std::string, Bar>& stepBars,
			std::unordered_map<std::string, double>& pricingMap,
			const std::vector<std::string>& activeFeedIDs,
			double initialBalance,
			double cagrLen,
			size_t maxBars,
			std::string batchID);

		void step();
		void runSteps(size_t n);
		void runToDate(const std::string& targetDate);
		void runAll();

		//getters for important states
		//these three are written as inline one-liners right here in the
		//header, since each just returns a single field with no other
		//logic - see dataFeed.hpp's totalBars() for the same pattern
		bool getIsFinished() const { return isFinished;}
		size_t getCurrentStep() const { return currentStep;}
		size_t getTotalSteps() const { return totalSteps;}
    private:
    	//important variables
	    std::string simID;
	    Account& tempAccount;
	    Broker& tempBroker;
	    std::unique_ptr<Strategy>& strategy;
	    Logger& tempLogger;
	    Metrics& calculator;
	    std::string batch;
	    //data holders
	    std::unordered_map<std::string, Data>& feeds;
	    std::unordered_map<std::string, Bar>& bars;
	    std::unordered_map<std::string, Bar>& tempBars;
	    std::unordered_map<std::string, double>& currPrices;

	    //for metrics
	    std::vector<std::string> feedIDs;
	    std::string primaryID;
	    double initBalance;
	    double cagrLength;

	    //steps progress
	    size_t currentStep;
	    size_t totalSteps;
	    bool isFinished;
};
