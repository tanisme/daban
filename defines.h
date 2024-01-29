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

#include <boost/bind/bind.hpp>
#include <boost/locale/encoding.hpp>
#include "TORA/TORATstpLev2MdApi.h"
#include "TORA/TORATstpTraderApi.h"

#define EXCHANGEID_SH "SSE"
#define EXCHANGEID_SZ "SZSE"
#define EXCHANGEID_UN "UNKNOW"

struct stOrder {
    TORALEV2API::TTORATstpLongSequenceType OrderNo;
    TORALEV2API::TTORATstpLongVolumeType Volume;
};

// vector
struct stPriceOrders {
    TORALEV2API::TTORATstpPriceType Price;
    std::vector<stOrder*> Orders;
};
typedef std::vector<std::vector<stPriceOrders>> MapOrderN;

struct stSecurity {
    TORASTOCKAPI::TTORATstpSecurityIDType SecurityID;
    TORASTOCKAPI::TTORATstpSecurityNameType SecurityName;
    TORASTOCKAPI::TTORATstpExchangeIDType ExchangeID;
    TORASTOCKAPI::TTORATstpSecurityTypeType SecurityType;
    TORASTOCKAPI::TTORATstpPriceType UpperLimitPrice;
    TORASTOCKAPI::TTORATstpPriceType LowerLimitPrice;
};

struct stHomebestOrder {
    TORALEV2API::TTORATstpLongVolumeType Volume;
    TORALEV2API::TTORATstpLSideType Side;
};

void Stringsplit(const std::string &str, const char split, std::vector<std::string> &res);
std::string GetTimeStr();
int GetNowTick();
int GetClockTick();
long long int GetUs();
int GetTotalIndex(TORASTOCKAPI::TTORATstpPriceType UpperLimitPrice, TORASTOCKAPI::TTORATstpPriceType LowerLimitPrice);
std::string toUTF8(std::string str);
std::string GetThisThreadID();

#endif //DABAN_DEFINES_H
