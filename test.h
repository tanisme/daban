#ifndef BALIBALI_TEST_H
#define BALIBALI_TEST_H

#include "defines.h"

namespace test {
    using namespace TORALEV2API;

    class Imitate {
    public:
        Imitate() = default;
        ~Imitate() = default;

        bool TestOrderBook(std::string& srcDataDir, std::string& watchsecurity);

    private:
        void OnRtnOrderDetail(TTORATstpSecurityIDType SecurityID, TTORATstpTradeBSFlagType Side, TTORATstpLongSequenceType OrderNO, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpExchangeIDType ExchangeID, TTORATstpLOrderStatusType OrderStatus);
        void OnRtnTransaction(TTORATstpSecurityIDType SecurityID, TTORATstpExchangeIDType ExchangeID, TTORATstpLongVolumeType TradeVolume, TTORATstpExecTypeType ExecType, TTORATstpLongSequenceType BuyNo, TTORATstpLongSequenceType SellNo, TTORATstpPriceType TradePrice);
        void OnRtnNGTSTick(TTORATstpSecurityIDType SecurityID, TTORATstpLTickTypeType TickType, TTORATstpLongSequenceType BuyNo, TTORATstpLongSequenceType SellNo, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpLSideType	Side);

        void ShowOrderBook(TTORATstpSecurityIDType SecurityID);
        void ShowUnfindOrder(TTORATstpSecurityIDType SecurityID);
        void InsertOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType orderNO, TTORATstpPriceType price, TTORATstpLongVolumeType Volume, TTORATstpLSideType side);
        bool ModifyOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongVolumeType tradeVolume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType side);
        void ResetOrder(TTORATstpSecurityIDType SecurityID, TTORATstpTradeBSFlagType side);
        void PostPrice(TTORATstpSecurityIDType SecurityID, TTORATstpPriceType price);
        void AddUnFindOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongVolumeType Volume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType side, int type = 0);
        void HandleUnFindOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType side);

        void SplitSecurityFile(std::string srcDataDir, bool isOrder);
        void SplitSecurityFileOrderQuot(std::string &dstDataDir, TTORATstpSecurityIDType SecurityID);
        void SplitSecurityFileTradeQuot(std::string &dstDataDir, TTORATstpSecurityIDType SecurityID);

        void BigFileOrderQuot(std::string &srcDataDir, TTORATstpSecurityIDType SecurityID);
        void BigFileTradeQuot(std::string &srcDataDir, TTORATstpSecurityIDType SecurityID);

        void LDParseSecurityFile(TTORATstpSecurityIDType CalSecurityID);
    private:
        int m_showPankouCount = 5;
        std::unordered_map<std::string, bool> m_watchSecurity;
        std::unordered_map<std::string, FILE*> m_watchOrderFILE;
        std::unordered_map<std::string, FILE*> m_watchTradeFILE;
        MapOrder m_orderBuy;
        MapOrder m_orderSell;
        MemoryPool<Order, 8192> m_orderPool;
        std::unordered_map<std::string, std::unordered_map<TTORATstpLongSequenceType, std::vector<stUnfindOrder*>>> m_unFindOrders;
        MemoryPool<stUnfindOrder, 8192> m_unFindOrderPool;
    };

}
#endif //BALIBALI_TEST_H
