#this is a standalone, one-off debug script - it's not called by GUI.py or
#core.py at all, you'd only ever run it by hand("python3 plotter.py") to
#eyeball one specific old simulation's equity curve as a quick sanity check
#
#heads up: the hardcoded path below points at an OLD output file naming
#scheme(test_don_dynamicData_1.csv, with the sim name and a counter baked
#into the filename) - the engine's current Logger writes a plain
#dynamicData.csv inside its own uniquely-named folder instead(see
#src/logger.cpp's exportCSV), so this script won't find a matching file
#for a sim run today without updating the path by hand first
import pandas as pd
import matplotlib.pyplot as plt

#load the csv file
path = "/Users/vivekvenigalla/Documents/VV_Active/03_PROJECTS/BacktestingEngine/output/test_batch/test_don/test_don_dynamicData_1.csv"
#pd.read_csv loads a csv file straight into a pandas DataFrame - basically
#a spreadsheet-like table you can slice/filter/plot without writing your
#own csv-parsing loop(compare to the manual line-by-line parsing the c++
#engine's own csvParser.cpp has to do, since c++ has no pandas equivalent)
loader = pd.read_csv(path)

#create a new collumn timestamp that converts the date string into a datetime object
#pd.to_datetime converts a column of date strings("2024-01-01") into real
#datetime objects - matplotlib needs real dates(not plain text) to draw a
#sensible, evenly-spaced time axis
loader["Timestamp"] = pd.to_datetime(loader['Date'])

#plt.subplots() creates a figure(the whole image) and an axes(the actual
#plot area to draw on) together in one call - fig is used later to save
#the image, ax is used to actually draw the lines onto it
fig, ax = plt.subplots(figsize = (8,4))

#drop any duplicates that can be caused by multiple tickers
#the engine's dynamicData.csv has one row per (date, ticker) pair, so a
#multi-ticker simulation logs the SAME date more than once - drop_duplicates
#keeps only the first row per date so each date is only plotted once
unique = loader.drop_duplicates(subset=["Date"])

#plot the total equity over time
#ax.plot draws one line - x values first, y values second. linestyle="--"
#on the second line makes it dashed, so equity(solid) and balance(dashed)
#stay visually distinct even though both are drawn in different colors already
ax.plot(unique["Timestamp"],unique["Equity"], label = "Equity", color = "green")
ax.plot(unique["Timestamp"],unique["Balance"], label = "Balance", color = "blue", linestyle = "--")
ax.set_ylabel("Money ($)")
ax.legend(loc = "upper left")
ax.set_title("Equity and Balance over Time")

#prevents overlapping
#tight_layout automatically adjusts spacing so labels/titles don't get cut
#off or overlap the plot area - purely cosmetic, no effect on the data
plt.tight_layout()

#save the plots in a png
plt.savefig("../plot_results/backtest_results3.png")
plt.show()
