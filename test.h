﻿#ifndef BALIBALI_TEST_H
#define BALIBALI_TEST_H

#include "defines.h"
#include "MemoryPool.h"

namespace test {
    using namespace TORALEV2API;

    class Imitate {
    public:
        Imitate() = default;
        ~Imitate() = default;

        bool TestOrderBook(std::string& srcDataDir, std::string& watchsecurity);

    private:
        void OnRtnOrderDetail(TTORATstpSecurityIDType SecurityID, TTORATstpTradeBSFlagType Side, TTORATstpLongSequenceType OrderNO, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpExchangeIDType ExchangeID, TTORATstpLOrderStatusType OrderStatus);
        void OnRtnTransaction(TTORATstpSecurityIDType SecurityID, TTORATstpExchangeIDType ExchangeID, TTORATstpLongVolumeType TradeVolume, TTORATstpExecTypeType ExecType, TTORATstpLongSequenceType BuyNo, TTORATstpLongSequenceType SellNo, TTORATstpPriceType TradePrice, TTORATstpTimeStampType	TradeTime);
        void OnRtnNGTSTick(TTORATstpSecurityIDType SecurityID, TTORATstpLTickTypeType TickType, TTORATstpLongSequenceType BuyNo, TTORATstpLongSequenceType SellNo, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpLSideType	Side, TTORATstpTimeStampType TickTime);

        void ShowOrderBook(TTORATstpSecurityIDType SecurityID);
        void ShowUnfindOrder(TTORATstpSecurityIDType SecurityID);
        void InsertOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType OrderNO, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpLSideType Side);
        bool ModifyOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongVolumeType Volume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType Side);
        void ResetOrder(TTORATstpSecurityIDType SecurityID, TTORATstpTradeBSFlagType Side);
        void FixOrder(TTORATstpSecurityIDType SecurityID, TTORATstpPriceType Price, TTORATstpTimeStampType Time);
        void AddUnFindOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongVolumeType Volume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType Side, int type = 0);
        void HandleUnFindOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType Side);

        void SplitSecurityFile(std::string srcDataDir, bool isOrder);
        void SplitSecurityFileOrderQuot(std::string &dstDataDir, TTORATstpSecurityIDType SecurityID);
        void SplitSecurityFileTradeQuot(std::string &dstDataDir, TTORATstpSecurityIDType SecurityID);

        void LDParseSecurityFile(TTORATstpSecurityIDType CalSecurityID);
    private:
        int m_showPankouCount = 5;
        std::unordered_map<std::string, bool> m_watchSecurity;
        std::unordered_map<std::string, FILE*> m_watchOrderFILE;
        std::unordered_map<std::string, FILE*> m_watchTradeFILE;
        MapOrder m_orderBuy;
        MapOrder m_orderSell;
        MemoryPool m_pool;
        std::unordered_map<std::string, std::unordered_map<TTORATstpLongSequenceType, std::vector<stUnfindOrder*>>> m_unFindOrders;
    };

}
#endif //BALIBALI_TEST_H
