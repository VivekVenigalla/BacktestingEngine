#include "./account.hpp"
#include "./structures.hpp"
#include "./broker.hpp"
#include "./account.hpp"
#include <unordered_map>

//this class allows tha main logic to evaluate the performance of the strategy


//provides static methods
class Metrics{
    public:
        double totalReturn(Account& user);

    private:
        Account& user;
        std::unordered_map<long int, Trade>& tradeHistory;

};