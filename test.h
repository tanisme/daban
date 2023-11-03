//
// Created by tangang on 2023/10/21.
//

#ifndef BALIBALI_TEST_H
#define BALIBALI_TEST_H

#include "TORA/TORATstpLev2ApiDataType.h"

namespace test {
    using namespace TORALEV2API;


    void ShowOrderBook(TTORATstpSecurityIDType SecurityID);

    void ResetOrder(TTORATstpSecurityIDType SecurityID, TTORATstpTradeBSFlagType Side);

    void DeleteOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType OrderNO);

    void InsertOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType OrderNO,
                     TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpLSideType Side);

    bool ModifyOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongVolumeType TradeVolume,
                     TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType Side);

    void OnRtnOrderDetail(TTORATstpSecurityIDType SecurityID, TTORATstpTradeBSFlagType Side,
                          TTORATstpLongSequenceType OrderNO, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume,
                          TTORATstpExchangeIDType ExchangeID, TTORATstpLOrderStatusType OrderStatus);

    void OnRtnTransaction(TTORATstpSecurityIDType SecurityID, TTORATstpExchangeIDType ExchangeID,
                          TTORATstpLongVolumeType TradeVolume, TTORATstpExecTypeType ExecType,
                          TTORATstpLongSequenceType BuyNo, TTORATstpLongSequenceType SellNo);

    void FixOrder(TTORATstpSecurityIDType securityID, TTORATstpPriceType TradePrice);

    void TestOrderBook();
}
#endif //BALIBALI_TEST_H
