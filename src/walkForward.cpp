#include "walkForward.hpp"

std::vector<WalkForwardWindow> generateWindows(
    const std::vector<std::string>& sortedDates,
    int inSampleBars,
    int outSampleBars,
    int stepBars
){
    std::vector<WalkForwardWindow> windows;

    //nothing sane to generate from a non-positive size/step - rather than
    //looping forever(a stepBars of 0 would never advance) or indexing
    //negatively, just hand back an empty result
    if(inSampleBars <= 0 || outSampleBars <= 0 || stepBars <= 0){
        return windows;
    }

    size_t windowSpan = static_cast<size_t>(inSampleBars) + static_cast<size_t>(outSampleBars);
    if(sortedDates.size() < windowSpan){
        return windows;
    }

    //start is the index of the FIRST bar in this window's in-sample half -
    //every iteration below carves one window out of [start, start+windowSpan)
    //and then slides start forward by stepBars for the next one
    size_t start = 0;
    while(start + windowSpan <= sortedDates.size()){
        WalkForwardWindow window;

        size_t inEndIndex = start + static_cast<size_t>(inSampleBars) - 1;
        size_t outStartIndex = start + static_cast<size_t>(inSampleBars);
        size_t outEndIndex = start + windowSpan - 1;

        window.inSampleStart = sortedDates[start];
        window.inSampleEnd = sortedDates[inEndIndex];
        window.outSampleStart = sortedDates[outStartIndex];
        window.outSampleEnd = sortedDates[outEndIndex];

        windows.push_back(window);

        start += static_cast<size_t>(stepBars);
    }

    return windows;
}
