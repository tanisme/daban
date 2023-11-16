#ifndef DABAN_DEFINES_H
#define DABAN_DEFINES_H

#include <ctime>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <thread>

#include <boost/bind/bind.hpp>
#include "TORA/TORATstpLev2MdApi.h"
#include "TORA/TORATstpTraderApi.h"

#define SERVICE_ADDSTRATEGYREQ      1   // 增加策略请求
#define SERVICE_ADDSTRATEGYRSP      2
#define SERVICE_DELSTRATEGYREQ      3   // 删除策略请求
#define SERVICE_DELSTRATEGYRSP      4
#define SERVICE_QRYSTRATEGYREQ      5   // 查询策略请求
#define SERVICE_QRYSTRATEGYRSP      6

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

struct stPriceOrders {
    TORALEV2API::TTORATstpPriceType Price;
    std::vector<stOrder*> Orders;
};

typedef std::unordered_map<std::string, std::vector<stPriceOrders> > MapOrder;

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

struct stUnfindOrder {
    TORALEV2API::TTORATstpSecurityIDType SecurityID;
    TORALEV2API::TTORATstpLongSequenceType OrderNo;
    TORALEV2API::TTORATstpLongVolumeType Volume;
    TORALEV2API::TTORATstpTradeBSFlagType Side;
    TORALEV2API::TTORATstpTimeStampType Time;
    int Type;   // 0-trade 1-deleteorder
};

void Stringsplit(const std::string &str, const char split, std::vector<std::string> &res);
std::string GetTimeStr();
int GetNowTick();
int GetClockTick();
void trim(std::string &s);
std::string GetThreadID();
long long int GetMs();

#endif //DABAN_DEFINES_H
