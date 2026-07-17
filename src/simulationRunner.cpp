#include "../include/simulationRunner.hpp"
#include <iostream>


SimulationRunner::SimulationRunner(const std::string& id,
									Account& accountRef,
									Broker& brokerRef,
									std::unique_ptr<Strategy>& strategyRef,
									Logger& loggerRef,
									Metrics& metricsRef,
									std::unordered_map<std::string, Data>& sharedFeeds,
									std::unordered_map<std::string, Bar>& sharedBars,
									std::unordered_map<std::string, Bar>& stepBars, //or tempBars
									std::unordered_map<std::string, double>& prices,
									const std::vector<std::string>& ids,
									double balance,
									double length,
									size_t maxBars)

									: simID(id),
									tempAccount(accountRef),
									tempBroker(brokerRef),
									strategy(strategyRef),
									tempLogger(loggerRef),
									calculator(metricsRef),
									feeds(sharedFeeds),
									bars(sharedBars),
									tempBars(stepBars),
									currPrices(prices),
									feedIDs(ids),
									initBalance(balance),
									cagrLength(length),
									currentStep(0),
									totalSteps(maxBars),
									isFinished(false){
		primaryID = feedIDs[0];

		//data check in case already ran through without resetting
		if (!feeds[primaryID].hasMoreData()){
			isFinished = true;
		}
}

void SimulationRunner::step(){
	//prevent any data leak
    if (isFinished) return;

    //update all bars and prices to the new feed bar
    for (const std::string& id : feedIDs){
        tempBars[id] = feeds[id].getBar();
        bars[id] = tempBars[id]; 
        currPrices[id] = tempBars[id].close;
    }

    //run broker
    //broker runs first to process any orders, especially limit and stop(prevents oversight)
    tempBroker.checkLoop();

    //load the bar onto the strategy and run its logic
    strategy->loadBar();
    strategy->runBar();
    
    //check the total equity and increment step
    double value = tempAccount.accountValue(currPrices);
    currentStep++;

    //output the current step and debugging info such as balance and total equity and date
    std::cout << "Sim[" << simID << "] Progress: " << currentStep << "/" << totalSteps
              << " | Date: " << tempBars[primaryID].date 
              << " | Balance: " << tempAccount.checkBalance() << "| Total Equity: " << value << "\n";

    //log data
    tempLogger.logSnapshot(tempBars[primaryID].date,tempBars,tempAccount.checkBalance(), value, tempAccount.returnPositions(),calculator.drawDown(value));
    
    //advance bar
    for (const auto& feedID : feedIDs){
    	feeds[feedID].nextBar();
    }
    //if the feed does not have more data than export the data
    if (!feeds[primaryID].hasMoreData()){
		isFinished = true;
		//output account and broker id for debugging and export data
	    std::cout << "Simulation [" << simID << "] finished" << "\n";
	    std::cout << "Account ID: " << tempAccount.id << "\n";
	    std::cout << "Broker ID: " << tempBroker.id << "\n";
	    tempLogger.exportData(simID, calculator, tempBroker.returnHistory(), currPrices, initBalance, cagrLength);
    }
}

//run n number of steps and also check if it is finished
void SimulationRunner::runSteps(size_t n) {
    for (size_t i = 0; i < n && !isFinished; ++i){
        step();
    }
}

//run to a specific date using the tempbar as a checker
void SimulationRunner::runToDate(const std::string& targetDate) {
    while (!isFinished && tempBars[primaryID].date < targetDate){
        step();
    }
}

//run the entire simulation
void SimulationRunner::runAll() {
    while (!isFinished){
        step();
    }
}