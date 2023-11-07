#ifndef DABAN_DEFINES_H
#define DABAN_DEFINES_H

#include <ctime>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <boost/bind/bind.hpp>
#include "TORA/TORATstpLev2MdApi.h"
#include "TORA/TORATstpTraderApi.h"

#define SERVICE_ADDSTRATEGYREQ      3   // 增加策略请求
#define SERVICE_ADDSTRATEGYRSP      4
#define SERVICE_DELSTRATEGYREQ      5   // 删除策略请求
#define SERVICE_DELSTRATEGYRSP      6
#define SERVICE_QRYSTRATEGYREQ      7   // 查询策略请求
#define SERVICE_QRYSTRATEGYRSP      8

typedef struct _stSecurity {
    TORALEV2API::TTORATstpSecurityIDType SecurityID;
    char SecurityName[81];
    TORALEV2API::TTORATstpExchangeIDType ExchangeID;
    int Status;
} stSubSecurity;

typedef struct _stStrategy {
    int idx;
    TORALEV2API::TTORATstpSecurityIDType SecurityID;
    int type;
    int status;
    struct stParams {
        double p1; double p2; double p3; double p4; double p5;
    };
    stParams params;
} stStrategy;

struct Order {
    TORALEV2API::TTORATstpLongSequenceType OrderNo;
    TORALEV2API::TTORATstpLongVolumeType Volume;
};

struct PriceOrders {
    TORALEV2API::TTORATstpPriceType Price;
    std::vector<Order*> Orders;
};

typedef std::unordered_map<std::string, std::vector<PriceOrders> > MapOrder; // TODO改内存池

struct stPostPrice {
    TORALEV2API::TTORATstpSecurityIDType SecurityID;
    TORALEV2API::TTORATstpPriceType AskPrice1;
    TORALEV2API::TTORATstpLongVolumeType AskVolume1;
    TORALEV2API::TTORATstpPriceType BidPrice1;
    TORALEV2API::TTORATstpLongVolumeType BidVolume1;
    TORALEV2API::TTORATstpPriceType TradePrice;
};

struct stSecurity_t {
    TORASTOCKAPI::TTORATstpSecurityIDType SecurityID;
    TORASTOCKAPI::TTORATstpSecurityNameType SecurityName;
    TORASTOCKAPI::TTORATstpExchangeIDType ExchangeID;
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
void trim(std::string &s);

#endif //DABAN_DEFINES_H
