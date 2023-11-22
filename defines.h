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
#include "TORA/TORATstpLev2MdApi.h"
#include "TORA/TORATstpTraderApi.h"

struct stStrategy {
    int idx;
    TORALEV2API::TTORATstpSecurityIDType SecurityID;
    int type;
    int status; // executed
    struct stParams {
        double p1; double p2; double p3; double p4; double p5;
    };
    stParams params;
};

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

// list
typedef std::unordered_map<std::string, std::map<int, std::vector<stOrder*>> > MapOrderL;

// map
struct stPriceOrdersMap {
    TORALEV2API::TTORATstpPriceType Price;
    std::unordered_map<TORALEV2API::TTORATstpLongSequenceType, std::list<stOrder*>> Orders;
};
typedef std::unordered_map<std::string, std::list<stPriceOrdersMap> > MapOrderM;

struct stPostPrice {
    TORALEV2API::TTORATstpSecurityIDType SecurityID;
    TORALEV2API::TTORATstpPriceType AskPrice1;
    TORALEV2API::TTORATstpLongVolumeType AskVolume1;
    TORALEV2API::TTORATstpPriceType BidPrice1;
    TORALEV2API::TTORATstpLongVolumeType BidVolume1;
    TORALEV2API::TTORATstpPriceType TradePrice;
};

struct stSecurity {
    TORASTOCKAPI::TTORATstpSecurityIDType SecurityID;
    TORASTOCKAPI::TTORATstpSecurityNameType SecurityName;
    TORASTOCKAPI::TTORATstpExchangeIDType ExchangeID;
    TORASTOCKAPI::TTORATstpSecurityTypeType SecurityType;
    TORASTOCKAPI::TTORATstpPriceType UpperLimitPrice;
    TORASTOCKAPI::TTORATstpPriceType LowerLimitPrice;
};

struct stNotifyData {
    int type;
    // OrderDetail
    TORALEV2API::TTORATstpSecurityIDType SecurityID;
    TORALEV2API::TTORATstpLSideType Side;
    TORALEV2API::TTORATstpLongSequenceType OrderNO;
    TORALEV2API::TTORATstpPriceType Price;
    TORALEV2API::TTORATstpLongVolumeType Volume;//TradeVolume
    TORALEV2API::TTORATstpExchangeIDType ExchangeID;
    TORALEV2API::TTORATstpLOrderStatusType OrderStatus;

    // Transaction
    TORALEV2API::TTORATstpExecTypeType ExecType;
    TORALEV2API::TTORATstpLongSequenceType	BuyNo;
    TORALEV2API::TTORATstpLongSequenceType	SellNo;

    // NGTS
    TORALEV2API::TTORATstpLTickTypeType TickType;
};

void Stringsplit(const std::string &str, const char split, std::vector<std::string> &res);
std::string GetTimeStr();
int GetNowTick();
int GetClockTick();
void trim(std::string &s);
std::string GetThreadID();
long long int GetUs();

#endif //DABAN_DEFINES_H
