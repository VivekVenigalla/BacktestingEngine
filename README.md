# Backtesting Engine by Vivek Venigalla

A high-performance algorithmic trading backtesting engine built from scratch in C++. This project simulates the execution of trading strategies using historical market data with optimized memory efficiency and realistic brokerage rules.

---

## Architecture Overview


* **CoreTypes.hpp:** Defines foundational structures (`Bar`, `Order`, `Position`, `Trade`) optimized for memory and cache management.
* **CSVParser:** Parses local CSV files into standard vectors in RAM, utilizing RAII to minimize memory allocations.
* **Account:** Serves as the primary ledger. It tracks balances and share positions using an unordered map for constant-time lookups while calculating total equity.
* **Broker:** Manages active orders(market, limit and stop). It utilizes an ID-keyed map to enable constant-time cancellations and handles initial order ingestion from the strategy layer.
* **Strategy:** Enables compile-time polymorphism so new child strategies can be developed, swapped, and managed via smart pointers

---

## Directory Structure 

```text
├── include/
│   ├── structures.hpp         #Memory structures (Bar, Order, Position, Trade)
│   ├── csvParser.hpp          #CSV file processing declarations
│   ├── account.hpp            #Portfolio management declarations
│   ├── broker.hpp             #Execution simulator declarations
│   ├── strategy.hpp           #Abstract strategy base declarations
│   ├── createStrategy.hpp     #Strategy creation declarations
│   ├── performanceEval.hpp    #Performance metrics declarations
│   ├── dataFeed.hpp           #Simulation data exporrt declarations
│   └── strategies/            #Subfolder for strategy declarations
├── src/
│   ├── csvParser.cpp          #CSV file processing definitions
│   ├── account.cpp            #Portfolio management definitions
│   ├── broker.cpp             #Execution simulator definitions
│   ├── strategy.cpp           #Abstract strategy base definitions
│   ├── csv_download.py        #Market data collection script
│   ├── plotter.py             #Graph plotting script
│   ├── createStrategy.cpp     #Strategy creation script
│   ├── performanceEval.cpp    #Performance metrics definitions
│   ├── dataFeed.cpp           #Simulation data exporrt definitions
│   ├── strategies/            #Subfolder for strategy definitions
│   └── main.cpp               #Simulation runner and verification
├── data/
│   ├── AAPL_interval_set.csv  #Historical market data
│   └── test.csv               #Simulation data 
└── CMakeLists.txt             #Build system configuration

```

---

## Current Test Framework

Currently the main.cpp serves as the unit testing for method development.


---

## Data Acquisition Pipeline

Before running a backtest, historical data must be downloaded. Since yfinance includes dependencies that can alter the environment, it is necessary to create a virtual environment.

To set up and activate the environment, run the following from the project root:

```bash
python3 -m venv yf_env
source yf_env/bin/activate
```
To install yfinance, run the following command:

```bash
pip install yfinance
```
To run the script and deactivate the environment, run the following commands:

```bash
python3 src/csv_download.py
deactivate
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

## Plotting

To plot any graphs using the plotter.py, make sure that matplotlib is installed. If not, activate the same environment and install the library

```bash
source yf_env/bin/activate
pip install matplotlib
```

Make sure that a test.csv exists and is filled with data before exeucting the script:

```bash
python3 src/plotter.py
deactivate
```



Will update with more information soon!