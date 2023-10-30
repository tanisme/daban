#include "TDImpl.h"
#include "CApplication.h"

#include <boost/bind/bind.hpp>

namespace PROTD {

    TDImpl::TDImpl(CApplication *pApp)
            : m_pApp(pApp) {
    }

    TDImpl::~TDImpl() {
        if (m_tdApi != nullptr) {
            m_tdApi->Join();
            m_tdApi->Release();
        }
    }

    bool TDImpl::Start() {
        m_tdApi = CTORATstpTraderApi::CreateTstpTraderApi();
        if (!m_tdApi) return false;

        m_tdApi->RegisterSpi(this);
        m_tdApi->RegisterFront((char *) m_pApp->m_TDAddr.c_str());
        m_tdApi->SubscribePrivateTopic(TORA_TERT_QUICK);
        m_tdApi->SubscribePublicTopic(TORA_TERT_QUICK);
        m_tdApi->Init();
        return true;
    }

    void TDImpl::OnFrontConnected() {
        printf("TDImpl::OnFrontConnected!!!\n");

        CTORATstpReqUserLoginField req = {0};
        req.LogInAccountType = TORA_TSTP_LACT_AccountID;
        strcpy(req.LogInAccount, m_pApp->m_TDAccount.c_str());
        strcpy(req.Password, m_pApp->m_TDPassword.c_str());
        strcpy(req.UserProductInfo, "notthisone");
        strcpy(req.TerminalInfo,
               "PC;IIP=123.112.154.118;IPORT=50361;LIP=192.168.118.107;MAC=54EE750B1713FCF8AE5CBD58;HD=TF655AY91GHRVL;@notthisone");
        m_tdApi->ReqUserLogin(&req, ++m_reqID);
    }

    void TDImpl::OnFrontDisconnected(int nReason) {
        printf("TDImpl::OnFrontDisconnected!!! Reason:%d\n", nReason);
        m_isLogined = false;
    }

    void TDImpl::OnRspUserLogin(CTORATstpRspUserLoginField *pRspUserLoginField, CTORATstpRspInfoField *pRspInfo, int nRequestID) {
        if (!pRspUserLoginField || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("TDImpl::OnRspUserLogin Failed!!! ErrMsg:%s\n", pRspInfo->ErrorMsg);
            return;
        }

        m_isLogined = true;
        m_pApp->m_ioc.post(boost::bind(&CApplication::TDOnRspUserLogin, m_pApp, *pRspUserLoginField));
        memcpy(&m_loginField, pRspUserLoginField, sizeof(m_loginField));

        CTORATstpQrySecurityField Req = {0};
        m_tdApi->ReqQrySecurity(&Req, ++m_reqID);
    }

    void TDImpl::OnRspQrySecurity(CTORATstpSecurityField *pSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (pSecurity) {
            m_pApp->m_ioc.post(boost::bind(&CApplication::TDOnRspQrySecurity, m_pApp, *pSecurity));
        }

        if (bIsLast) {
            printf("TDImpl::OnRspQrySecurity Success!!!\n");
            CTORATstpQryOrderField req = {0};
            m_tdApi->ReqQryOrder(&req, 0);
        }
    }

    void TDImpl::OnRspQryOrder(CTORATstpOrderField *pOrder, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (pOrder) {
            m_pApp->m_ioc.post(boost::bind(&CApplication::TDOnRspQryOrder, m_pApp, *pOrder));
        }

        if (bIsLast) {
            printf("TDImpl::OnRspQryOrder Success!!!\n");
            CTORATstpQryTradeField Req = {0};
            m_tdApi->ReqQryTrade(&Req, 0);
        }
    }

    void TDImpl::OnRspQryTrade(CTORATstpTradeField *pTrade, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (pTrade) {
            m_pApp->m_ioc.post(boost::bind(&CApplication::TDOnRspQryTrade, m_pApp, *pTrade));
        }

        if (bIsLast) {
            printf("TDImpl::OnRspQryTrade Success!!!\n");
            CTORATstpQryPositionField Req = {0};
            m_tdApi->ReqQryPosition(&Req, 0);
        }
    }

    void TDImpl::OnRspQryPosition(CTORATstpPositionField *pPosition, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (pPosition) {
            m_pApp->m_ioc.post(boost::bind(&CApplication::TDOnRspQryPosition, m_pApp, *pPosition));
        }

        if (bIsLast) {
            printf("TDImpl::OnRspQryPosition Success!!!\n");
            CTORATstpQryTradingAccountField Req = {0};
            m_tdApi->ReqQryTradingAccount(&Req, 0);
        }
    }

    void TDImpl::OnRspQryTradingAccount(CTORATstpTradingAccountField *pTradingAccount, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (pTradingAccount) {
            m_pApp->m_ioc.post(boost::bind(&CApplication::TDOnRspQryTradingAccount, m_pApp, *pTradingAccount));
        }

        if (bIsLast) {
            printf("TDImpl::OnRspQryTradingAccount Success!!!\n");
        }
    }

    int TDImpl::OrderInsert(TTORATstpSecurityIDType SecurityID, TTORATstpDirectionType Direction,
                            TTORATstpVolumeType VolumeTotalOriginal, TTORATstpPriceType LimitPric) {
        CTORATstpInputOrderField req = {0};

        strcpy(req.SecurityID, SecurityID);
        req.Direction = Direction;
        req.VolumeTotalOriginal = VolumeTotalOriginal;
        req.LimitPrice = LimitPric;
        req.OrderPriceType = TORA_TSTP_OPT_LimitPrice;
        req.TimeCondition = TORA_TSTP_TC_GFD;
        req.VolumeCondition = TORA_TSTP_VC_AV;
        req.OrderRef = 1;
        return m_tdApi->ReqOrderInsert(&req, ++m_reqID);
    }

    void TDImpl::OnRspOrderInsert(CTORATstpInputOrderField *pInputOrderField, CTORATstpRspInfoField *pRspInfoField, int nRequestID) {
        if (!pInputOrderField || !pRspInfoField) return;
        if (pRspInfoField->ErrorID > 0) {
            printf("TDImpl::OnRspOrderInsert Failed!!! ErrMsg:%s\n", pRspInfoField->ErrorMsg);
            return;
        }
    }

    void TDImpl::OnErrRtnOrderInsert(CTORATstpInputOrderField *pInputOrderField, CTORATstpRspInfoField *pRspInfoField, int nRequestID) {
        printf("TDImpl::OnErrRtnOrderInsert\n");
    }

    void TDImpl::OnRtnOrder(CTORATstpOrderField *pOrder) {
        if (!pOrder) return;

    }

    void TDImpl::OnRtnTrade(CTORATstpTradeField *pTrade) {
        if (!pTrade) return;


    }

    int TDImpl::OrderCancel() {
        CTORATstpInputOrderActionField req = {0};
        req.ActionFlag = TORA_TSTP_AF_Delete;
        return m_tdApi->ReqOrderAction(&req, ++m_reqID);
    }

    void TDImpl::OnRspOrderAction(CTORATstpInputOrderActionField *pInputOrderActionField, CTORATstpRspInfoField *pRspInfoField, int nRequestID) {
    }

    void TDImpl::OnErrRtnOrderAction(CTORATstpInputOrderActionField *pInputOrderActionField, CTORATstpRspInfoField *pRspInfoField, int nRequestID) {
    }

}
