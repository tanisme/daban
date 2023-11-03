//
// Created by tangang on 2023/10/21.
//


#ifndef BALIBALI_TEST_H
#define BALIBALI_TEST_H

#include "TORA/TORATstpLev2ApiDataType.h"

namespace test {
    using namespace TORALEV2API;

    void ShowOrderBook(TTORATstpSecurityIDType SecurityID);
    void HandleOrderDetail(TTORATstpSecurityIDType SecurityID, TTORATstpExchangeIDType ExchangeID, TTORATstpLSideType Side, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpLongSequenceType OrderNO, TTORATstpLOrderStatusType OrderStatus);
    void HandleTransaction(TTORATstpSecurityIDType SecurityID, TTORATstpExchangeIDType ExchangeID, TTORATstpPriceType TradePrice, TTORATstpLongVolumeType TradeVolume, TTORATstpLongSequenceType BuyNo, TTORATstpLongSequenceType SellNo, TTORATstpExecTypeType ExecType);
    void HandleNGTSTick(TTORATstpSecurityIDType SecurityID, TTORATstpExchangeIDType ExchangeID, TTORATstpLSideType Side,TTORATstpLongSequenceType BuyNo, TTORATstpLongSequenceType SellNo,TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpLTickTypeType TickType);
    void InsertOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType orderNO, TTORATstpPriceType price, TTORATstpLongVolumeType Volume, TTORATstpLSideType side);
    bool ModifyOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongVolumeType tradeVolume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType side);
    void ResetOrder(TTORATstpSecurityIDType SecurityID, TTORATstpTradeBSFlagType side);

    void TestOrderBook();
}
#endif //BALIBALI_TEST_H
