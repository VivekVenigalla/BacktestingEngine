import yfinance as yf
import pandas as pd
import os

#Activating virtual environment
#source yf_env/bin/activate

#Path to where to upload the data
PATH_TO_DOWNLOAD = "../data"
#add more tickers if needed to upload data
#tickers = ["AAPL", "GOOGL", "SPY", "NVDA"]
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

def downloadData(ticker, start_date, end_date, interval_set):
    print(ticker)
    print(start_date)
    print(end_date)
    print(interval_set)
    temp = yf.Ticker(ticker)
    data = temp.history(start = start_date, end = end_date, interval = interval_set, auto_adjust = True, prepost = False, actions = False)
    #adjust time rows to be formatted in &Y-%m-%d
    data.index = data.index.strftime("%Y-%m-%d")

    data_dir = os.path.join("..", "data")
    os.makedirs(data_dir, exist_ok=True)
    filename = ticker + "_" + interval_set + "_" + start_date + "_" + end_date + ".csv"
    filepath = os.path.join(data_dir, filename)
    data.to_csv(filepath, index = True)