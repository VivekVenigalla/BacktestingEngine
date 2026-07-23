# Backtesting Engine by Vivek Venigalla

A high-performance algorithmic trading backtesting engine built from scratch in C++, with a dear pygui frontend. This project simulates the execution of trading strategies using historical market data with optimized memory efficiency and realistic brokerage rules, with a seamless ui system that allows endless customizability and exploration

---

## Architecture Overview

### C++ Core Engine

* **CoreTypes:** Defines foundational structures (`Bar`, `Order`, `Position`, `Trade`) optimized for memory and cache management.
* **CSV Parser:** Parses local CSV files into standard vectors in RAM, utilizing RAII to minimize memory allocations.
* **Account:** Serves as the primary ledger. It tracks balances and share positions using an unordered map for constant-time lookups while calculating total equity.
* **Broker:** Manages active orders(market, limit and stop). It utilizes an ID-keyed map to enable constant-time cancellations and handles initial order ingestion from the strategy layer.
* **Strategy:** Enables compile-time polymorphism so new child strategies can be developed, swapped, and managed via smart pointers
* **Simulation Runner:** Orchestrates batch simulation processes with different configurations

### Python UI and Workbench(Dear PyGUI)

* **Interactive Node Editor Workbench:** A visual node-based editor allowing users to map and configure Accounts, Brokers, Data Feeds, and Strategies dynamically.
* **Batch Configuration Management:** Supports saving, parsing, validating, and loading complex multi-simulation batch schemas

---

## Directory Structure 

```text
├── include/
│   ├── structures.hpp         		#Memory structures (Bar, Order, Position, Trade)
│   ├── csvParser.hpp          		#CSV file processing declarations
│   ├── account.hpp            		#Portfolio management declarations
│   ├── broker.hpp             		#Execution simulator declarations
│   ├── strategy.hpp           		#Abstract strategy base declarations
│   ├── createStrat.hpp        		#Strategy creation declarations
│   ├── performanceEval.hpp    		#Performance metrics declarations
│   ├── dataFeed.hpp           		#Simulation data exporrt declarations
│   ├── simulationRunner.cpp   		#Simulation manager declarations
│   ├── logger.hpp             		#Simulation metrics management declarations
│   └── strategies/            		#Subfolder for strategy declarations
├── src/
│   ├── structures.hpp         		#Memory structures print methods
│   ├── csvParser.cpp          		#CSV file processing definitions
│   ├── account.cpp            		#Portfolio management definitions
│   ├── broker.cpp             		#Execution simulator definitions
│   ├── strategy.cpp           		#Abstract strategy base definitions
│   ├── csv_download.py        		#Market data collection script
│   ├── plotter.py             		#Graph plotting script
│   ├── createStrat.cpp        		#Strategy creation script
│   ├── performanceEval.cpp    		#Performance metrics definitions
│   ├── dataFeed.cpp           		#Simulation data exporrt definitions
│   ├── simulationRunner.cpp   		#Simulation manager definitions
│   ├── logger.cpp             		#Simulation metrics management definitions
│   ├── strategies/            		#Subfolder for strategy definitions
│   ├── main.cpp               		#Simulation runner and verification
│   ├── core.py                		#JSON import and GUI integration
│   └── GUI.py                 		#Graphical interface
├── config/
│   ├── batchConfig/           		#Configs for each batch
│   ├── accountConfig.json     		#Seperate account configs
│   ├── brokerConfig.json           #Seperate broker configs
│   ├── feedConfig.json             #Seperate feed configs
│   └── simulationBlueprints.json   #Seperate simulation configs
├── data/							#Feed Data
├── output/							#Output Data
└── CMakeLists.txt             		#Build system configuration

```

---

## Current Test Framework

Currently the main.cpp serves as the unit testing for method development. However the GUI.py is currently in active development and will soon be the primary method of testing new features.


---

## Virtual Environment Setup

Before running a backtest, historical data must be downloaded. Yfinance, the library used to download historical data, includes dependencies that can alter the environment. Dear PyGui also presents a similar challenge, so it is necessary to create a virtual environment. On MacOS, this project relies on conda, so please have conda and miniforge installed if using macOS.

To set up and activate the environment, run the following from the project root:

Linux:

```bash
python3 -m venv trading_env
source trading_env/bin/activate
```

Mac:

```bash
conda create --name trading_env python=3.11
conda activate trading_env
```


To install yfinance and dearpygui, run the following command while the environment is active:

```bash
pip install yfinance
pip install dearpygui
```


## Compilation and Execution

This project is built using CMake. Ensure you have CMake(3.10 or higher) and a modern C++17 compiler.

To compile run the following commands(this will create a new directory called build and write all build files here):

```bash
mkdir build
cd build
cmake ..
```
To compile the code, execute the following command:

```bash
cmake --build .
```
This will return a executable by the name of runny.

To run the code, execute the following command:

```bash
./runny
```
NOTE: This code will return a csv file in the data directory with the simulation results

## GUI Run

To use the GUI, make sure dearpygui is installed
```bash
cd src
python3 GUI.py
deactivate
```

More information about using the GUI will come out soon!