#pragma once
#include <string>
#include <vector>

//walk-forward analysis answers a different question than Monte Carlo does:
//Monte Carlo resamples ONE completed run's own returns to see how much luck
//shaped it, but it never proves the strategy's PARAMETERS would still work
//on price data they weren't tuned against. walk-forward does that by
//rolling a window across history - tune/evaluate on an "in-sample" stretch,
//then immediately re-check the SAME parameters on the very next, unseen
//"out-of-sample" stretch, then slide forward and repeat. a strategy whose
//out-of-sample results consistently look much worse than its in-sample
//results is a classic sign of overfitting to one particular stretch of
//history, rather than having found a real, repeatable edge
struct WalkForwardWindow{
    //all four are date strings(e.g "2024-03-15"), matching the same
    //YYYY-MM-DD keys Data's internal std::map<std::string, Bar> is already
    //sorted by - both ranges are INCLUSIVE on both ends
    std::string inSampleStart = "";
    std::string inSampleEnd = "";
    std::string outSampleStart = "";
    std::string outSampleEnd = "";
};

//sortedDates: every available bar date, in chronological order(a plain
//std::vector<std::string> - Day 7 is what actually slices Data's bars by
//the date bounds this produces; this function only computes WHERE the
//bounds fall, it never touches Data or Bar itself)
//
//inSampleBars/outSampleBars: how many bars each window's two halves span
//
//stepBars: how far the START of the window advances between one window and
//the next - a stepBars smaller than inSampleBars means consecutive windows'
//in-sample ranges overlap(a denser, more expensive sweep); stepBars equal
//to inSampleBars means they never overlap
//
//returns every window that fully fits within sortedDates - a window
//that would run off the end of the available dates simply isn't included,
//rather than being generated with missing/truncated data
std::vector<WalkForwardWindow> generateWindows(
    const std::vector<std::string>& sortedDates,
    int inSampleBars,
    int outSampleBars,
    int stepBars
);
