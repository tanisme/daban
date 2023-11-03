#ifndef TEST_MDL2IMPL_H
#define TEST_MDL2IMPL_H

#include "defines.h"

class CApplication;

namespace PROMD {
    using namespace TORALEV2API;

    class MDL2Impl : public CTORATstpLev2MdSpi {
    public:
        explicit MDL2Impl(CApplication *pApp, TTORATstpExchangeIDType ExchangeID);
        ~MDL2Impl();

        bool Start(bool isTest = true);
        int ReqMarketData(TTORATstpSecurityIDType SecurityID, TTORATstpExchangeIDType ExchangeID, int type, bool isSub = true);
        void ShowOrderBook(TTORATstpSecurityIDType SecurityID);
        static const char *GetExchangeName(TTORATstpExchangeIDType ExchangeID);

        bool IsLogined() const { return m_isLogined; }
        CTORATstpLev2MdApi *GetApi() const { return m_pApi; }

    public:
        void OnFrontConnected() override;
        void OnFrontDisconnected(int nReason) override;
        void OnRspUserLogin(CTORATstpRspUserLoginField *pRspUserLogin, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRspSubMarketData(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRspUnSubMarketData(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRspSubOrderDetail(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRspUnSubOrderDetail(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRspSubTransaction(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRspUnSubTransaction(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRspSubNGTSTick(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRspUnSubNGTSTick(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRtnMarketData(CTORATstpLev2MarketDataField *pDepthMarketData, const int FirstLevelBuyNum, const int FirstLevelBuyOrderVolumes[], const int FirstLevelSellNum, const int FirstLevelSellOrderVolumes[]) override;
        void OnRtnOrderDetail(CTORATstpLev2OrderDetailField *pOrderDetail) override;
        void OnRtnTransaction(CTORATstpLev2TransactionField *pTransaction) override;
        void OnRtnNGTSTick(CTORATstpLev2NGTSTickField *pTick) override;

    private:
        void InsertOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType orderNO, TTORATstpPriceType price, TTORATstpLongVolumeType Volume, TTORATstpLSideType side);
        bool ModifyOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongVolumeType tradeVolume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType side);
        void ResetOrder(TTORATstpSecurityIDType SecurityID, TTORATstpTradeBSFlagType side);
        void PostPrice(TTORATstpSecurityIDType SecurityID, TTORATstpPriceType price);

        void FixOrder(TTORATstpSecurityIDType SecurityID, TTORATstpPriceType TradePrice);
        void AddUnFindTrade(TTORATstpSecurityIDType SecurityID, TTORATstpLongVolumeType tradeVolume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType side);
        void HandleUnFindTrade(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType side);

    private:
        int m_reqID = 1;
        bool m_isLogined = false;
        CTORATstpLev2MdApi *m_pApi = nullptr;
        CApplication *m_pApp = nullptr;
        TTORATstpExchangeIDType m_exchangeID;
        MapOrder m_orderBuy;
        MapOrder m_orderSell;
        std::unordered_map<std::string, stPostPrice> m_postMDL2;
        std::unordered_map<std::string, std::map<TTORATstpLongSequenceType, std::vector<Order>> > m_unFindBuyTrades;
        std::unordered_map<std::string, std::map<TTORATstpLongSequenceType, std::vector<Order>> > m_unFindSellTrades;
    };

}

#endif //TEST_MDL2IMPL_H
