#this script downloads real historical stock price data and saves it as a
#csv file the c++ engine's csvParser.cpp can read - it's the ONLY place in
#this project that talks to the internet(via the yfinance library), the c++
#side never fetches data itself, it only ever reads whatever csv files
#already exist in data/
import yfinance as yf
import pandas as pd
import os

#Activating virtual environment
#source yf_env/bin/activate

#Path to where to upload the data
PATH_TO_DOWNLOAD = "../data"
#add more tickers if needed to upload data
#tickers = ["AAPL", "GOOGL", "SPY", "NVDA"]
#everything inside the '''...''' below is a triple-quoted string, which
#python treats as one big multi-line piece of text rather than real code -
#that's what makes this whole block effectively "commented out"(a poor
#man's block comment, since python has no /* */ like c++ does). it's left
#over example code showing how downloadData() below used to be called
#directly for a fixed list of tickers, before the GUI started calling it
#per-ticker instead
'''tickers = ["AAPL"]

#other contraints such as start and end time, intervals and more

#we will use a 10 year window
start_date = "2015-1-11"
end_date = "2025-1-11"

interval_set = "1d"
auto_adjust_set = True
pre_post_set = False
actions_set = False
date_set = True

for t in tickers:
    temp = yf.Ticker(t)
    data = temp.history(start = start_date, end = end_date, interval = interval_set, auto_adjust = auto_adjust_set, prepost = False, actions = False)
    data.index = data.index.strftime("%Y-%m-%d")
    #loop through the
    print(data.head(3))
    data.to_csv("./data/" + t + "_" + interval_set + ".csv", index = date_set)
    print("Sucessful data upload")'''

#this is what the GUI actually calls when you add a new data feed - see
#GUI.py's feed-creation modal for the caller
def downloadData(ticker, start_date, end_date, interval_set):
    #yf.Ticker(ticker) creates a handle to that stock on yahoo finance;
    #.history(...) is the actual network call that fetches the price data
    #and hands it back as a pandas DataFrame(a table), indexed by date
    temp = yf.Ticker(ticker)
    data = temp.history(start = start_date, end = end_date, interval = interval_set, auto_adjust = True, prepost = False, actions = False)
    #adjust time rows to be formatted in &Y-%m-%d
    #yfinance's dates come back as full datetime objects with a timezone
    #attached - strftime("%Y-%m-%d") reformats them down to a plain
    #"2024-01-01"-style string, matching what csvParser.cpp expects to parse
    data.index = data.index.strftime("%Y-%m-%d")

    #os.path.join builds a file path using the right slash direction for
    #whatever operating system this runs on("../data/x.csv" on mac/linux,
    #"..\\data\\x.csv" on windows) - same portability idea as c++'s
    #fs::path "/" operator in logger.cpp
    data_dir = os.path.join("..", "data")
    #exist_ok=True means "don't error out if the folder is already there" -
    #without it, os.makedirs throws on every call after the first
    os.makedirs(data_dir, exist_ok=True)
    filename = ticker + "_" + interval_set + "_" + start_date + "_" + end_date + ".csv"
    filepath = os.path.join(data_dir, filename)
    data.to_csv(filepath, index = True)
