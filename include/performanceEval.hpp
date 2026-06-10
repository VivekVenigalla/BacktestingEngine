#include "./account.hpp"
#include "./structures.hpp"
#include "./broker.hpp"
#include <unordered_map>
#include <cmath>
//this class allows tha main logic to evaluate the performance of the strategy


//provides static methods
class Metrics{
    public:
        Metrics(Account& a,std::unordered_map<long int, Trade>& tH, std::vector<std::string> all);
        //returns the total return from the initial balance to all position value
        double totalReturn(double initial, double currPrice);
        double cagr(double initial, double currPrice, int years);

    private:
        Account& user;
        std::unordered_map<long int, Trade>& tradeHistory;
        std::vector<std::string> allTickers;

};