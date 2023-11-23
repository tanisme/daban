﻿#ifndef TEST_MDL2IMPL_H
#define TEST_MDL2IMPL_H

#include "defines.h"
#include "MemoryPool.h"
#include <memory>
#include <boost/lockfree/spsc_queue.hpp>

class CApplication;

namespace PROMD {
    using namespace TORALEV2API;

    class MDL2Impl : public CTORATstpLev2MdSpi {
    public:
        explicit MDL2Impl(CApplication *pApp, TTORATstpExchangeIDType ExchangeID);
        ~MDL2Impl();

        bool Start(bool isTest, int version);
        int ReqMarketData(TTORATstpSecurityIDType SecurityID, TTORATstpExchangeIDType ExchangeID, int type, bool isSub = true);
        void ShowOrderBookV(TTORATstpSecurityIDType SecurityID);
        void ShowOrderBookM(TTORATstpSecurityIDType SecurityID);
        void ShowHandleSpeed();
        static const char *GetExchangeName(TTORATstpExchangeIDType ExchangeID);
        void SetVersion(int val) { m_version = val; }

        bool IsInited() const { return m_isInited; }
        CTORATstpLev2MdApi *GetApi() const { return m_pApi; }

        void OrderDetail(TTORATstpSecurityIDType SecurityID, TTORATstpTradeBSFlagType Side, TTORATstpLongSequenceType OrderNO, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpExchangeIDType ExchangeID, TTORATstpLOrderStatusType OrderStatus);
        void Transaction(TTORATstpSecurityIDType SecurityID, TTORATstpExchangeIDType ExchangeID, TTORATstpLongVolumeType TradeVolume, TTORATstpExecTypeType ExecType, TTORATstpLongSequenceType BuyNo, TTORATstpLongSequenceType SellNo, TTORATstpPriceType TradePrice, TTORATstpTimeStampType	TradeTime);
        void NGTSTick(TTORATstpSecurityIDType SecurityID, TTORATstpLTickTypeType TickType, TTORATstpLongSequenceType BuyNo, TTORATstpLongSequenceType SellNo, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpLSideType	Side, TTORATstpTimeStampType TickTime);

    public:
        void OnFrontConnected() override;
        void OnFrontDisconnected(int nReason) override;
        void OnRspUserLogin(CTORATstpRspUserLoginField *pRspUserLogin, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRspSubOrderDetail(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRspUnSubOrderDetail(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRspSubTransaction(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRspUnSubTransaction(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRspSubNGTSTick(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRspUnSubNGTSTick(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRtnOrderDetail(CTORATstpLev2OrderDetailField *pOrderDetail) override;
        void OnRtnTransaction(CTORATstpLev2TransactionField *pTransaction) override;
        void OnRtnNGTSTick(CTORATstpLev2NGTSTickField *pTick) override;

    private:
        // vector
        void InsertOrderV(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType OrderNO, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpLSideType Side);
        void ModifyOrderV(TTORATstpSecurityIDType SecurityID, TTORATstpLongVolumeType Volume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType Side);
        void PostPriceV(TTORATstpSecurityIDType SecurityID, TTORATstpPriceType Price);
        // map
        void InsertOrderM(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType OrderNO, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpLSideType Side);
        void ModifyOrderM(TTORATstpSecurityIDType SecurityID, TTORATstpLongVolumeType Volume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType Side);
        void PostPriceM(TTORATstpSecurityIDType SecurityID, TTORATstpPriceType Price);

        void HandleData(stNotifyData* data);
        void Run();

    private:
        int m_version = 1;
        int m_reqID = 1;
        bool m_isInited = false;
        CTORATstpLev2MdApi *m_pApi = nullptr;
        CApplication *m_pApp = nullptr;
        TTORATstpExchangeIDType m_exchangeID;
        MapOrderV m_orderBuyV;
        MapOrderV m_orderSellV;
        MapOrderL m_orderBuyL;
        MapOrderL m_orderSellL;
        MapOrderM m_orderBuyM;
        MapOrderM m_orderSellM;

        std::atomic_bool m_stop;
        boost::lockfree::spsc_queue<stNotifyData*, boost::lockfree::capacity<1024>> m_data;
        std::list<stNotifyData*> m_dataList;
        std::mutex m_dataMtx;
        std::thread* m_pthread;

        MemoryPool m_pool;
        long long int m_addQueueCount = 0;
        long long int m_delQueueCount = 0;
        long long int m_handleTick = 0;
    };

}

#endif //TEST_MDL2IMPL_H
