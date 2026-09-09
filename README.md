# Backtesting Engine by Vivek Venigalla

A high-performance algorithmic trading backtesting engine built from scratch in C++, with a dear pygui frontend. This project simulates the execution of trading strategies using historical market data with optimized memory efficiency and realistic brokerage rules, with a seamless ui system that allows endless customizability and exploration

---

## Architecture Overview

### C++ Core Engine

* **CoreTypes:** Defines foundational structures (`Bar`, `Order`, `Position`, `Trade`) optimized for memory and cache management.
* **CSV Parser:** Parses local CSV files into standard vectors in RAM, utilizing RAII to minimize memory allocations.
* **Account:** Serves as the primary ledger. It tracks balances and share positions using an unordered map for constant-time lookups while calculating total equity.
* **Broker:** Manages active orders(market, limit and stop). It utilizes an ID-keyed map to enable constant-time cancellations and handles initial order ingestion from the strategy layer.
* **Strategy:** Enables runtime polymorphism (via virtual functions and `std::unique_ptr`) so new child strategies can be developed, swapped, and managed interchangeably by the simulation runner.
* **Performance Metrics:** Computes total return, CAGR, drawdown (current and historical max), Sharpe/Sortino ratios, and trade-level win rate/profit factor for a finished or in-progress simulation.
* **Logger:** Records a bar-by-bar equity curve during a run and exports it, the trade history, and the computed metrics to CSV/JSON files once a simulation completes.
* **Simulation Runner:** Orchestrates batch simulation processes with different configurations, stepping through historical bars one at a time (or all at once) while avoiding lookahead bias.

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
│   ├── dataFeed.hpp           		#Simulation data feed declarations
│   ├── simulationRunner.hpp   		#Simulation manager declarations
│   ├── logger.hpp             		#Simulation metrics management declarations
│   ├── projectPaths.hpp.in    		#Template for a build-time-generated header (project root path)
│   └── strategies/            		#Subfolder for strategy declarations
├── src/
│   ├── structures.cpp         		#Memory structures print methods
│   ├── csvParser.cpp          		#CSV file processing definitions
│   ├── account.cpp            		#Portfolio management definitions
│   ├── broker.cpp             		#Execution simulator definitions
│   ├── strategy.cpp           		#Abstract strategy base definitions
│   ├── csv_download.py        		#Market data collection script
│   ├── plotter.py             		#Graph plotting script
│   ├── createStrat.cpp        		#Strategy creation script
│   ├── performanceEval.cpp    		#Performance metrics definitions
│   ├── dataFeed.cpp           		#Simulation data feed definitions
│   ├── simulationRunner.cpp   		#Simulation manager definitions
│   ├── logger.cpp             		#Simulation metrics management definitions
│   ├── strategies/            		#Subfolder for strategy definitions
│   ├── main.cpp               		#Batch-driven simulation entry point
│   ├── core.py                		#JSON import and GUI integration
│   └── GUI.py                 		#Graphical interface
├── tests/
│   ├── CMakeLists.txt         		#Test executable target, registered with ctest
│   ├── fixtures/              		#Small sample CSV files used only by the test suite
│   └── test_*.cpp             		#Catch2 unit tests, one file per module
├── config/
│   ├── batchConfig/           		#Configs for each batch
│   ├── accountConfig.json     		#Seperate account configs
│   ├── brokerConfig.json           #Seperate broker configs
│   ├── feedConfig.json             #Seperate feed configs
│   └── strategyBlueprints.json     #Seperate strategy configs
├── data/							#Feed Data
├── output/							#Output Data
├── plot_results/					#Saved matplotlib chart images (from plotter.py)
├── requirements.txt				#Python dependencies
└── CMakeLists.txt             		#Build system configuration

```

---

## Current Test Framework

The C++ engine has an automated unit test suite built with [Catch2](https://github.com/catchorg/Catch2) v3, wired into the CMake build (see `tests/CMakeLists.txt` and `enable_testing()` in the root `CMakeLists.txt`). Every module (`Account`, `Broker`, each strategy, `Metrics`, `Logger`, `Data`, `StrategyFactory`, etc.) has its own `test_*.cpp` file under `tests/`, and `ctest` auto-discovers every individual test case. See [Running Tests](#running-tests) below for how to build and run the suite.

`GUI.py` remains the primary way to interactively exercise the engine end-to-end (build a batch visually, run it, inspect results), but it is not part of the automated test suite.

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

Alternatively, `requirements.txt` covers the rest of the Python dependencies used by `csv_download.py`/`GUI.py` (pandas, matplotlib, etc.) and can be installed in one step with `pip install -r requirements.txt` — note it does not currently include `dearpygui`, so that still needs to be installed separately as shown above.


## Compilation and Execution

This project is built using CMake (3.15 or higher) and a C++20 compiler.

Before compiling, [nlohmann/json](https://github.com/nlohmann/json) (3.10.5 or newer) must be installed as a system package, since `find_package(nlohmann_json)` is required by the build:

```bash
brew install nlohmann-json
```

(On Linux, install the `nlohmann-json3-dev` package via your package manager, or use vcpkg/conan.)

To compile run the following commands(this will create a new directory called build and write all build files here):

```bash
mkdir build
cd build
cmake ..
```
NOTE: the first `cmake ..` also downloads and builds [Catch2](https://github.com/catchorg/Catch2) (the test framework) via CMake's `FetchContent`, so it needs internet access the first time and will take noticeably longer than later configures.

To compile the code, execute the following command:

```bash
cmake --build .
```
This will produce two executables: `runny` (the engine) and `unit_tests` (the test suite).

To run a batch simulation, pass the path to a batch config JSON file as the one command-line argument:

```bash
./runny ../config/batchConfig/test_batch.json
```
NOTE: running `./runny` with no arguments now prints a usage message and exits instead of running anything. Results are written under `output/<batch_id>/<simulation_id>/` as `dynamicData.csv` (bar-by-bar equity curve), `tradeData.csv` (trade log), and `metricData.json` (computed performance metrics) — not to the `data/` directory, which only ever holds input price data.

### Running Tests

From the same `build` directory:

```bash
ctest --output-on-failure
```
This runs every test case registered by the Catch2 suite and reports pass/fail per test. You can also run the `unit_tests` binary directly (e.g. `./tests/unit_tests "[account]"` to run just one module's tests, using the tag on that file's `TEST_CASE`s).

## GUI Run

To use the GUI, make sure dearpygui is installed
```bash
cd src
python3 GUI.py
deactivate
```

More information about using the GUI will come out soon!
