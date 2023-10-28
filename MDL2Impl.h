#ifndef TEST_MDL2IMPL_H
#define TEST_MDL2IMPL_H

#include "defines.h"

class CApplication;
namespace PROMD {

    using namespace TORALEV2API;

    class MDL2Impl : public CTORATstpLev2MdSpi {
    public:
        MDL2Impl(CApplication *App, TTORATstpExchangeIDType ExchangeID);
        ~MDL2Impl();

        bool Start();
        CTORATstpLev2MdApi *GetApi() const { return m_Api; }
        static const char *GetExchangeName(TTORATstpExchangeIDType ExchangeID);
        static const char *GetSide(TTORATstpLSideType Side);
        int ReqMarketData(TTORATstpSecurityIDType SecurityID, TTORATstpExchangeIDType ExchangeID, int type, bool isSub = true);
        void ShowOrderBook();
        void ShowFixOrderBook(TTORATstpSecurityIDType securityID);

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
        void InsertOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType OrderNO, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpLSideType Side);
        void ModifyOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongVolumeType TradeVolume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType Side);
        void DeleteOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType OrderNO);
        void ResetOrder(TTORATstpSecurityIDType SecurityID, TTORATstpTradeBSFlagType Side);
        void FixOrder(TTORATstpSecurityIDType SecurityID, TTORATstpPriceType Price);
        void PostPrice(TTORATstpSecurityIDType SecurityID);

    private:
        int m_reqID = 1;
        bool m_isLogined = false;
        CTORATstpLev2MdApi *m_Api = nullptr;
        CApplication *m_App = nullptr;
        TTORATstpExchangeIDType m_ExchangeID;
        MapOrder m_OrderBuy;
        MapOrder m_OrderSell;
        std::unordered_map<std::string, int> m_SubSecurityIDs;
        std::unordered_map<std::string, stPostPrice> m_postPrice;
    };

}

#endif //TEST_MDL2IMPL_H
