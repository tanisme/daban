#ifndef TEST_MDL2IMPL_H
#define TEST_MDL2IMPL_H

#include "defines.h"

class CApplication;
namespace PROMD {

    using namespace TORALEV2API;
    class MDL2Impl : public CTORATstpLev2MdSpi {
    public:
        explicit MDL2Impl(CApplication *pApp, TTORATstpExchangeIDType exchangeID);
        ~MDL2Impl();

        bool Start(bool useTcp = true);
        bool IsLogined() const { return m_isLogined; }
        CTORATstpLev2MdApi *GetApi() const { return m_pApi; }

        int ReqMarketData(TTORATstpSecurityIDType securityID, TTORATstpExchangeIDType exchangeID, int type, bool isSub = true);
        void ShowFixOrderBook(TTORATstpSecurityIDType securityID);
        void GenOrderBook(TTORATstpSecurityIDType securityID);
        static const char *GetExchangeName(TTORATstpExchangeIDType exchangeID);

    public:
        void OnFrontConnected() override;
        void OnFrontDisconnected(int nReason) override;
        void OnRspUserLogin(CTORATstpRspUserLoginField *pRspUserLogin, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRspSubMarketData(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRspUnSubMarketData(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRtnMarketData(CTORATstpLev2MarketDataField *pDepthMarketData, const int FirstLevelBuyNum, const int FirstLevelBuyOrderVolumes[], const int FirstLevelSellNum, const int FirstLevelSellOrderVolumes[]) override;
        void OnRspSubTransaction(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRspUnSubTransaction(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRtnTransaction(CTORATstpLev2TransactionField *pTransaction) override;
        void OnRspSubOrderDetail(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRspUnSubOrderDetail(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRtnOrderDetail(CTORATstpLev2OrderDetailField *pOrderDetail) override;
        void OnRspSubNGTSTick(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRspUnSubNGTSTick(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRtnNGTSTick(CTORATstpLev2NGTSTickField *pTick) override;

    private:
        void InsertOrder(TTORATstpSecurityIDType securityID, TTORATstpLongSequenceType orderNO, TTORATstpPriceType price, TTORATstpLongVolumeType Volume, TTORATstpLSideType side);
        bool ModifyOrder(TTORATstpSecurityIDType securityID, TTORATstpLongVolumeType tradeVolume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType side);
        void FixOrder(TTORATstpSecurityIDType securityID, TTORATstpPriceType TradePrice);
        void ResetOrder(TTORATstpSecurityIDType securityID, TTORATstpTradeBSFlagType side);
        void AddUnFindTrade(TTORATstpSecurityIDType securityID, TTORATstpLongVolumeType tradeVolume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType side);
        void HandleUnFindTrade(TTORATstpSecurityIDType securityID, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType side);
        void PostPrice(TTORATstpSecurityIDType securityID, TTORATstpPriceType price);

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

        std::unordered_map<std::string, std::string> m_orderBookStr;
    };

}

#endif //TEST_MDL2IMPL_H
