//
// Created by tanisme on 2023/10/28.
//

#ifndef DABAN_DEFINES_H
#define DABAN_DEFINES_H

#include <vector>
#include <unordered_map>

#include "TORA/TORATstpLev2MdApi.h"

typedef struct _stSecurity {
    TORALEV2API::TTORATstpSecurityIDType SecurityID;
    char SecurityName[81];
    TORALEV2API::TTORATstpExchangeIDType ExchangeID;
    int Status;
} stSubSecurity;

typedef struct stStrategy_s {
    int idx;
    TORALEV2API::TTORATstpSecurityIDType SecurityID;
    TORALEV2API::TTORATstpExchangeIDType ExchangeID;
} stStrategy_t;

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
