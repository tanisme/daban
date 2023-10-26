#ifndef TEST_MDL2IMPL_H
#define TEST_MDL2IMPL_H

#include <unordered_map>
#include "TORA/TORATstpLev2MdApi.h"

class CApplication;
namespace PROMD {
    using namespace TORALEV2API;

    struct Order {
        TTORATstpLongSequenceType OrderNo;
        TTORATstpLongVolumeType Volume;
    };

    struct PriceOrders {
        TTORATstpPriceType Price;
        std::vector<Order> Orders;
    };

    typedef std::unordered_map<std::string, std::vector<PriceOrders> > MapOrder;

    class MDL2Impl : public CTORATstpLev2MdSpi {
    public:
        MDL2Impl(CApplication *pApp, TTORATstpExchangeIDType exchangeID);

        ~MDL2Impl();

        bool Start();

        CTORATstpLev2MdApi *GetApi() const { return m_mdApi; }

        static const char *GetExchangeName(TTORATstpExchangeIDType exchangeID);

        static const char *GetSide(TTORATstpLSideType Side);

        static const char *GetOrderStatus(TTORATstpLOrderStatusType OrderStatus);

        int ReqMarketData(TTORATstpSecurityIDType security, TTORATstpExchangeIDType exchangeID, int type,
                          bool isSub = true);

        // ½Ó¿Ú
        void OnFrontConnected() override;

        void OnFrontDisconnected(int nReason) override;

        void OnRspUserLogin(CTORATstpRspUserLoginField *pRspUserLogin, CTORATstpRspInfoField *pRspInfo, int nRequestID,
                            bool bIsLast) override;

        void OnRspSubMarketData(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo,
                                int nRequestID, bool bIsLast) override;

        void OnRspUnSubMarketData(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo,
                                  int nRequestID, bool bIsLast) override;

        void OnRtnMarketData(CTORATstpLev2MarketDataField *pDepthMarketData, const int FirstLevelBuyNum,
                             const int FirstLevelBuyOrderVolumes[], const int FirstLevelSellNum,
                             const int FirstLevelSellOrderVolumes[]) override;

        void OnRspSubTransaction(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo,
                                 int nRequestID, bool bIsLast) override;

        void OnRspUnSubTransaction(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo,
                                   int nRequestID, bool bIsLast) override;

        void OnRtnTransaction(CTORATstpLev2TransactionField *pTransaction) override;

        void OnRspSubOrderDetail(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo,
                                 int nRequestID, bool bIsLast) override;

        void OnRspUnSubOrderDetail(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo,
                                   int nRequestID, bool bIsLast) override;

        void OnRtnOrderDetail(CTORATstpLev2OrderDetailField *pOrderDetail) override;

    private:
        void
        InsertOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType OrderNO, TTORATstpPriceType Price,
                    TTORATstpLongVolumeType Volume, TTORATstpLSideType Side);

        void ModifyOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongVolumeType TradeVolume,
                         TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType Side);

        void DeleteOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType OrderNO);

        void ResetOrder(TTORATstpSecurityIDType SecurityID, TTORATstpTradeBSFlagType Side);

        void ShowOrderBook();

        void FixOrder(TTORATstpSecurityIDType SecurityID, TTORATstpPriceType Price);

    private:
        int m_reqID = 1;
        bool m_isLogined = false;
        CTORATstpLev2MdApi *m_mdApi = nullptr;
        CApplication *m_pApp = nullptr;
        TTORATstpExchangeIDType m_exchangeID;

        std::unordered_map<std::string, int> m_subSecuritys;

        MapOrder m_OrderBuy;
        MapOrder m_OrderSell;
    };

}

#endif //TEST_MDL2IMPL_H
