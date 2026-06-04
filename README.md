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
│   └── strategies/            #Subfolder for strategy declarations
├── src/
│   ├── csvParser.cpp          #CSV file processing definitions
│   ├── account.cpp            #Portfolio management definitions
│   ├── broker.cpp             #Execution simulator definitions
│   ├── strategy.cpp           #Abstract strategy base definitions
│   ├── csv_download.py        #Market data collection script
│   ├── strategies/            #Subfolder for strategy definitions
│   └── main.cpp               #Simulation runner and verification
├── data/
│   └── AAPL_interval_set.csv  #Historical market data 
└── CMakeLists.txt             #Build system configuration

```

---

## Current Test Framework

Currently the main.cpp serves as the unit testing for method development.


---

## Data Acquisition Pipeline

Before running a backtest, historical data must be generated within a virtual environment.

To set up the data environment, run the following from the project root:

```bash
python3 -m venv yf_env
source yf_env/bin/activate
pip install yfinance
python3 src/csv_download.py
deactivate
```

## Compilation

This project is built using CMake. Ensure you have CMake(3.10 or higher) and a modern C++17 compiler.

To compile run the following commands(this will create a new directory called build and write all build files here):

```bash
mkdir build
cd build
cmake ..
cmake --build .

```
To run the code, execute the following command:

```bash
./runny

```

Will update with more information soon!