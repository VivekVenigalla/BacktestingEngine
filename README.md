# Backtesting Engine by Vivek Venigalla

A high-performance algorithmic trading backtesting engine built from scratch in C++, with a dear pygui frontend. This project simulates the execution of trading strategies using historical market data with optimized memory efficiency and realistic brokerage rules, with a seamless ui system that allows endless customizability and exploration

---

## Architecture Overview

### C++ Core Engine

* **CoreTypes:** Defines foundational structures (`Bar`, `Order`, `Position`, `Trade`) optimized for memory and cache management.
* **CSV Parser:** Parses local CSV files into standard vectors in RAM, utilizing RAII to minimize memory allocations.
* **Account:** Serves as the primary ledger. It tracks balances and share positions using an unordered map for constant-time lookups while calculating total equity.
* **Broker:** Manages active orders (market, limit, stop, and stop-limit) and supports short-selling with a cash-collateral guardrail. It utilizes an ID-keyed map to enable constant-time cancellations and handles initial order ingestion from the strategy layer.
* **Strategy:** Enables runtime polymorphism (via virtual functions and `std::unique_ptr`) so new child strategies can be developed, swapped, and managed interchangeably by the simulation runner.
* **Performance Metrics:** Computes total return, CAGR, drawdown (current and historical max), Sharpe/Sortino ratios, and trade-level win rate/profit factor for a finished or in-progress simulation.
* **Logger:** Records a bar-by-bar equity curve during a run and exports it, the trade history, and the computed metrics to CSV/JSON files once a simulation completes.
* **Simulation Runner:** Orchestrates batch simulation processes with different configurations, stepping through historical bars one at a time (or all at once) while avoiding lookahead bias. It has a silent mode (no console output, no export files) used by the validation tools below, which run it many times.

### Validation Tools

Three opt-in tools judge a strategy by more than "the final equity looked fine on one run". Each is switched on per simulation with its own block in the batch config (see [Validation Tools Usage](#validation-tools-usage)) and writes one extra file next to that simulation's normal output. A simulation without the block runs exactly as it always has.

* **Monte Carlo (`MonteCarloSimulator`):** Resamples ONE finished run's own period returns to show how much luck shaped the result. *Bootstrap* resampling draws returns with replacement and compounds them into a distribution of final equities; *shuffled-order* resampling reorders the same returns to show how much worse the drawdown could have been from sequence alone. It is pure arithmetic over the return series and never re-runs the bar-by-bar simulation, so thousands of runs are near-instant. Seedable for reproducibility.
* **Walk-Forward (`generateWindows`, `runWalkForward`):** Rolls an in-sample window and the out-of-sample window right after it across the history, running the SAME strategy parameters on each (every run is a fresh Account/Broker/Strategy over a copy of that date slice, made by `Data::sliceBars`). Out-of-sample results that are much worse than in-sample are the classic sign of overfitting.
* **Optimization (`ParameterGrid`, `runOptimization`):** Takes a list of candidate values per parameter, runs every combination of them as its own full simulation over the whole dataset, and ranks the combinations by a chosen objective (`sharpe`, `total_return` or `cagr`).

### Python UI and Workbench(Dear PyGUI)

* **Interactive Node Editor Workbench:** A visual node-based editor allowing users to map and configure Accounts, Brokers, Data Feeds, and Strategies dynamically.
* **Batch Configuration Management:** Supports saving, parsing, validating, and loading complex multi-simulation batch schemas
* **Validation Controls and Results Tabs:** Each strategy node has opt-in controls for Monte Carlo, Walk-Forward and Optimization, and the results dashboard has a matching tab for each (see [GUI Run](#gui-run)).

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
│   ├── monteCarlo.hpp         		#Monte Carlo resampling declarations
│   ├── walkForward.hpp        		#Walk-forward windows and driver declarations
│   ├── optimizer.hpp          		#Parameter grid, optimization driver, ranking and export declarations
│   ├── dataFeed.hpp           		#Simulation data feed declarations
│   ├── simulationRunner.hpp   		#Simulation manager declarations
│   ├── logger.hpp             		#Simulation metrics management declarations
│   ├── pathUtils.hpp          		#Data-feed CSV path resolution declarations
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
│   ├── monteCarlo.cpp         		#Monte Carlo resampling definitions
│   ├── walkForward.cpp        		#Walk-forward windows and driver definitions
│   ├── optimizer.cpp          		#Parameter grid, optimization driver, ranking and export definitions
│   ├── dataFeed.cpp           		#Simulation data feed definitions
│   ├── simulationRunner.cpp   		#Simulation manager definitions
│   ├── logger.cpp             		#Simulation metrics management definitions
│   ├── pathUtils.cpp          		#Data-feed CSV path resolution definitions
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
NOTE: running `./runny` with no arguments now prints a usage message and exits instead of running anything. Results are written under `output/<batch_id>/<simulation_id>/` as `dynamicData.csv` (bar-by-bar equity curve), `tradeData.csv` (trade log, including each trade's realized P&L, limit price, and bar date), and `metricData.json` (computed performance metrics) — not to the `data/` directory, which only ever holds input price data. A simulation that opts into a validation tool also gets `monteCarloResults.json`, `walkForwardResults.json`, and/or `optimizationResults.csv` plus `optimizationSummary.json` in that same folder.

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

The GUI is organized into a few viewports: a landing hub for browsing past batches, a config manager for editing Accounts/Brokers/Data Feeds/Strategies, the node-editor workbench for visually building a batch, and a results dashboard once a batch has run.

The results dashboard's left pane shows Performance Metrics (total return, CAGR), a Risk & Benchmark Metrics section (Sharpe ratio, Sortino ratio, max drawdown, benchmark return), and Trade Statistics (total/successful/unsuccessful trades, win rate, profit factor). The right pane is tabbed: a **Charts** tab with the equity/cash-balance curve (annotated with buy/sell markers at each filled trade) and an underwater drawdown chart, and a **Trade Log** tab listing every order the broker processed, with realized P&L color-coded green/red. Three more tabs appear for simulations that opted into a validation tool, and show a short "no data" note otherwise: **Monte Carlo** (final-equity percentiles and histogram, plus shuffled-order max-drawdown percentiles), **Walk-Forward** (a table of each window's in-sample vs out-of-sample return and Sharpe, with the % degradation between them), and **Optimization** (a table of every parameter combination, ranked best-first, that can be re-sorted by clicking any column header).

In the workbench, each strategy node has an "Enable Monte Carlo" checkbox (plus run count), an "Enable Walk-Forward" checkbox (plus in-sample, out-sample and step bar counts), and an "Enable Optimization" checkbox with an objective dropdown and an "Opt Grid (json)" box, e.g. `{"fast_period": [5, 10, 20], "slow_period": [30, 50]}` (pre-filled with a small grid over the strategy's first numeric parameter). An invalid grid is reported when the batch is exported.

## Validation Tools Usage

Each block goes on a simulation's entry in the batch config's `simulations` list, and each is skipped entirely when absent or when `"enabled"` is `false`:

```json
{
  "id": "sma_instance_1", "strategy": "sma", "account_link": "basicAccount", "broker_link": "basicBroker",
  "feeds": ["AAPL_1D_2015-1-11_2025-1-11"],
  "parameters": {"fast_period": 10, "slow_period": 50},
  "monte_carlo": {"enabled": true, "runs": 1000, "seed": 42},
  "walk_forward": {"enabled": true, "in_sample_bars": 504, "out_sample_bars": 126, "step_bars": 126},
  "optimization": {"enabled": true, "objective": "sharpe",
                   "grid": {"fast_period": [5, 10, 20], "slow_period": [30, 50, 100]}}
}
```

* `monte_carlo`: `runs` defaults to 1000; omit `seed` for a different random draw each time. Writes `monteCarloResults.json`.
* `walk_forward`: window sizes are in bars (defaults 252 / 63 / 63). Every run starts cold, so an indicator needing N bars of history can't trade until N bars into each window - keep windows comfortably longer than the strategy's longest lookback. Writes `walkForwardResults.json`.
* `optimization`: every combination of the grid is run (the grid above is 3 x 3 = 9 simulations, so cost grows quickly with more parameters or values). Parameters not in the grid keep the values from the simulation's own `parameters`. `objective` is `sharpe` (default), `total_return` or `cagr`. Writes `optimizationResults.csv` (one ranked row per combination) and `optimizationSummary.json` (the best combination). A bad objective or grid prints a message and skips the optimization without affecting the normal run.

A note on reading the results: grid search picks whatever worked best on this one stretch of history, so on its own it rewards overfitting. Pair it with walk-forward (does a setting still work on data it wasn't picked on?) and Monte Carlo (how much of the result was luck?) before trusting a "best" combination.

## In Progress

The validation toolkit (Monte Carlo, walk-forward, optimization) is complete and tested end to end. Next on the roadmap is a broader library of new strategies - RSI mean-reversion, MACD, ATR volatility breakout with a trailing stop, VWAP reversion, a multi-timeframe confirmation filter, and a multi-asset dual-momentum strategy - each to be judged with these tools rather than on a single backtest.
