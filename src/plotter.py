import pandas as pd
import matplotlib.pyplot as plt

#load the csv file
path = "/Users/vivekvenigalla/Documents/VV_Active/03_PROJECTS/BacktestingEngine/output/don_AAPL_1D_1/don_AAPL_1D_1_dynamicData_1.csv"
loader = pd.read_csv(path)

#create a new collumn timestamp that converts the date string into a datetime object 
loader["Timestamp"] = pd.to_datetime(loader['Date'])

fig, ax = plt.subplots(figsize = (8,4))

#drop any duplicates that can be caused by multiple tickers
unique = loader.drop_duplicates(subset=["Date"])

#plot the total equity over time
ax.plot(unique["Timestamp"],unique["Equity"], label = "Equity", color = "green")
ax.plot(unique["Timestamp"],unique["Balance"], label = "Balance", color = "blue", linestyle = "--")
ax.set_ylabel("Money ($)")
ax.legend(loc = "upper left")
ax.set_title("Equity and Balance over Time")

#prevents overlapping
plt.tight_layout()

#save the plots in a png
plt.savefig("../plot_results/backtest_results3.png")
plt.show()
