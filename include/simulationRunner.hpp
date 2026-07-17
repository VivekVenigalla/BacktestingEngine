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

class SimulationRunner {


	public:
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
			size_t maxBars);

		void step();
		void runSteps(size_t n);
		void runToDate(const std::string& targetDate);
		void runAll();

		//getters for important states
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
