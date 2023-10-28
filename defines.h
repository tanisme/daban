//
// Created by tanisme on 2023/10/28.
//

#ifndef DABAN_DEFINES_H
#define DABAN_DEFINES_H

#include <vector>
#include <unordered_map>

#include "TORA/TORATstpLev2MdApi.h"
#include "TORA/TORATstpTraderApi.h"

#define SERVICE_QRYSUBSECURITYREQ   1
#define SERVICE_QRYSUBSECURITYRSP   2
#define SERVICE_ADDSTRATEGYREQ      3
#define SERVICE_ADDSTRATEGYRSP      4
#define SERVICE_DELSTRATEGYREQ      5
#define SERVICE_DELSTRATEGYRSP      6
#define SERVICE_QRYSTRATEGYREQ      7
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
    std::vector<Order> Orders;
};

typedef std::unordered_map<std::string, std::vector<PriceOrders> > MapOrder;

struct stPostPrice {
    TORALEV2API::TTORATstpSecurityIDType SecurityID;
    TORALEV2API::TTORATstpPriceType AskPrice1;
    TORALEV2API::TTORATstpLongVolumeType AskVolume1;
    TORALEV2API::TTORATstpPriceType BidPrice1;
    TORALEV2API::TTORATstpLongVolumeType BidVolume1;
};

#endif //DABAN_DEFINES_H
