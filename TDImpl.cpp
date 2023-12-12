#include "TDImpl.h"
#include "CApplication.h"

#include <boost/bind/bind.hpp>

namespace PROTD {

    TDImpl::TDImpl(CApplication *pApp) : m_pApp(pApp) {
    }

    TDImpl::~TDImpl() {
        if (m_pApi) {
            m_pApi->Join();
            m_pApi->Release();
        }
    }

    void TDImpl::Start() {
        m_pApi = CTORATstpTraderApi::CreateTstpTraderApi();
        m_pApi->RegisterSpi(this);
        m_pApi->RegisterFront((char *) m_pApp->m_TDAddr.c_str());
        m_pApi->SubscribePrivateTopic(TORA_TERT_QUICK);
        m_pApi->SubscribePublicTopic(TORA_TERT_QUICK);
        m_pApi->Init();
    }

    void TDImpl::OnFrontConnected() {
        printf("TD::OnFrontConnected!!!\n");
        CTORATstpReqUserLoginField Req = {0};
        Req.LogInAccountType = TORA_TSTP_LACT_AccountID;
        strcpy(Req.LogInAccount, m_pApp->m_TDAccount.c_str());
        strcpy(Req.Password, m_pApp->m_TDPassword.c_str());
        m_pApi->ReqUserLogin(&Req, ++m_reqID);
    }

    void TDImpl::OnFrontDisconnected(int nReason) {
        printf("TD::OnFrontDisconnected!!! Reason:%d\n", nReason);
        m_isInited = false;
    }

    void TDImpl::OnRspUserLogin(CTORATstpRspUserLoginField *pRspUserLoginField, CTORATstpRspInfoField *pRspInfo, int nRequestID) {
        if (!pRspUserLoginField || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("TD::OnRspUserLogin Failed!!! ErrMsg:%s\n", pRspInfo->ErrorMsg);
            return;
        }

        CTORATstpQrySecurityField Req = {0};
        m_pApi->ReqQrySecurity(&Req, ++m_reqID);
    }

    void TDImpl::OnRspQrySecurity(CTORATstpSecurityField *pSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (pSecurity && pSecurity->bPriceLimit > 0) {
            if (pSecurity->SecurityType == PROTD::TORA_TSTP_STP_SHAShares ||
                pSecurity->SecurityType == PROTD::TORA_TSTP_STP_SHKC ||
                pSecurity->SecurityType == PROTD::TORA_TSTP_STP_SZCDR ||
                pSecurity->SecurityType == PROTD::TORA_TSTP_STP_SZGEM ||
                pSecurity->SecurityType == PROTD::TORA_TSTP_STP_SZMainAShares) {
                m_pApp->m_ioc.post(boost::bind(&CApplication::TDOnRspQrySecurity, m_pApp, *pSecurity));
            }
        }

        if (bIsLast) {
            m_isInited = true;
            m_pApp->m_ioc.post(boost::bind(&CApplication::TDOnInitFinished, m_pApp));
        }
    }

}
