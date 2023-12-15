#ifndef DABAN_DEFINES_H
#define DABAN_DEFINES_H

#include <ctime>
#include <fstream>
#include <sstream>
#include <map>
#include <unordered_map>
#include <vector>
#include <list>
#include <thread>
#include <atomic>

#include <boost/bind/bind.hpp>
#include "TORA/TORATstpLev2MdApi.h"
#include "TORA/TORATstpTraderApi.h"

struct stOrder {
    TORALEV2API::TTORATstpLongSequenceType OrderNo;
    TORALEV2API::TTORATstpLongVolumeType Volume;
};

// vector
struct stPriceOrders {
    TORALEV2API::TTORATstpPriceType Price;
    std::vector<stOrder*> Orders;
};
typedef std::unordered_map<std::string, std::vector<stPriceOrders> > MapOrderV;

struct stSecurity {
    TORASTOCKAPI::TTORATstpSecurityIDType SecurityID;
    TORASTOCKAPI::TTORATstpExchangeIDType ExchangeID;
    TORASTOCKAPI::TTORATstpPriceType UpperLimitPrice;
    TORASTOCKAPI::TTORATstpPriceType LowerLimitPrice;
};

struct stHomebestOrder {
    TORALEV2API::TTORATstpSecurityIDType SecurityID;
    TORALEV2API::TTORATstpLongSequenceType OrderNO;
    TORALEV2API::TTORATstpLongVolumeType Volume;
    TORALEV2API::TTORATstpLSideType Side;
};

void Stringsplit(const std::string &str, const char split, std::vector<std::string> &res);
std::string GetTimeStr();
int GetNowTick();
int GetClockTick();
void trim(std::string &s);
std::string GetThreadID();
long long int GetUs();

#endif //DABAN_DEFINES_H
