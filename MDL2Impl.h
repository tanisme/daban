#ifndef TEST_MDL2IMPL_H
#define TEST_MDL2IMPL_H

#include "defines.h"
#include "MemoryPool.h"
#include <memory>

class CApplication;

namespace PROMD {
    using namespace TORALEV2API;

    class MDL2Impl : public CTORATstpLev2MdSpi {
    public:
        explicit MDL2Impl(CApplication *pApp, TTORATstpExchangeIDType ExchangeID);
        ~MDL2Impl();

        void Start(bool isTest);
        bool IsInited() const { return m_isInited; }
        CTORATstpLev2MdApi *Api() const { return m_pApi; }
        static const char *GetExchangeName(TTORATstpExchangeIDType ExchangeID);

    public:
        void OnFrontConnected() override;
        void OnFrontDisconnected(int nReason) override;
        void OnRspUserLogin(CTORATstpRspUserLoginField *pRspUserLogin, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRtnOrderDetail(CTORATstpLev2OrderDetailField *pOrderDetail) override;
        void OnRtnTransaction(CTORATstpLev2TransactionField *pTransaction) override;
        void OnRtnNGTSTick(CTORATstpLev2NGTSTickField *pTick) override;

    private:
        int m_reqID = 1;
        bool m_isInited = false;
        CTORATstpLev2MdApi *m_pApi = nullptr;
        CApplication *m_pApp = nullptr;
        TTORATstpExchangeIDType m_exchangeID;
    };

}

#endif //TEST_MDL2IMPL_H
