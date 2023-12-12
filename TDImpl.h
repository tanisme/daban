#ifndef TEST_TDIMPL_H
#define TEST_TDIMPL_H

#include "defines.h"
#include "MemoryPool.h"

class CApplication;

namespace PROTD {
    using namespace TORASTOCKAPI;

    class TDImpl : public CTORATstpTraderSpi {
    public:
        explicit TDImpl(CApplication *pApp);
        ~TDImpl();

        void Start();
        bool IsInited() const { return m_isInited; }
        CTORATstpTraderApi *Api() const { return m_pApi; }

    public:
        void OnFrontConnected() override;
        void OnFrontDisconnected(int nReason) override;
        void OnRspUserLogin(CTORATstpRspUserLoginField *pRspUserLoginField, CTORATstpRspInfoField *pRspInfoField, int nRequestID) override;
        void OnRspQrySecurity(CTORATstpSecurityField *pSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    private:
        int m_reqID = 1;
        bool m_isInited = false;
        CTORATstpTraderApi *m_pApi = nullptr;
        CApplication *m_pApp = nullptr;
    };

}

#endif //TEST_TDIMPL_H
