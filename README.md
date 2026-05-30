# Backtesting Engine by Vivek Venigalla

A high-performance algorithmic trading backtesting engine built from scratch in C++. This project simulates the execution of trading strategies using historical market data with optimized memory efficiency and realistic brokerage rules.

---

## Architecture Overview


* **CoreTypes.hpp:** Defines foundational structures (`Bar`, `Order`, `Position`, `Trade`) optimized for memory and cache management.
* **CSVParser:** Parses local CSV files into standard vectors in RAM, utilizing RAII to minimize memory allocations.
* **Account:** Serves as the primary ledger. It tracks balances and share positions using an unordered map for constant-time lookups while calculating total equity.
* **Broker:** Manages active orders(market, limit and stop). It utilizes an ID-keyed map to enable constant-time cancellations and handles initial order ingestion from the strategy layer.

---

## Directory Structure

```text
├── include/
│   ├── CoreTypes.hpp    # Memory structures (Bar, Order, Position, Trade)
│   ├── CSVParser.hpp    # File processing interface
│   ├── Account.hpp      # Portfolio tracking definitions
│   └── Broker.hpp       # Execution simulator declarations
├── src/
│   ├── CSVParser.cpp    # String parsing mechanics
│   ├── Account.cpp      # Ledger math and cost-basis management
│   └── Broker.cpp       # Ingestion and constructor implementations
├── data/
│   └── AAPL_daily.csv   # Historical market data generated via Python 
└── main.cpp             # Unit test verification suite

```

---

## Current Test Framework

Currently the main.cpp serves as the unit testing for method development.


---

## Compilation

This project is built using CMake

To compile run the following commands in a new directory called build

```bash
mkdir build
cd build
cmake ..

```
To run the code, execute the following command, which will provide the executable(runny)

```bash
cmake --build .
./runny

```

Will update with more information soon!