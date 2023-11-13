#include "TDImpl.h"
#include "CApplication.h"

#include <boost/bind/bind.hpp>

namespace PROTD {

    TDImpl::TDImpl(CApplication *pApp)
            : m_pApp(pApp) {
    }

    TDImpl::~TDImpl() {
        if (m_pApi) {
            m_pApi->Join();
            m_pApi->Release();
        }
    }

    bool TDImpl::Start() {
        m_pApi = CTORATstpTraderApi::CreateTstpTraderApi();
        if (!m_pApi) return false;

        m_pApi->RegisterSpi(this);
        m_pApi->RegisterFront((char *) m_pApp->m_TDAddr.c_str());
        m_pApi->SubscribePrivateTopic(TORA_TERT_QUICK);
        m_pApi->SubscribePublicTopic(TORA_TERT_QUICK);
        m_pApi->Init();
        return true;
    }

    void TDImpl::OnFrontConnected() {
        printf("TD::OnFrontConnected!!!\n");

        CTORATstpReqUserLoginField req = {0};
        req.LogInAccountType = TORA_TSTP_LACT_AccountID;
        strcpy(req.LogInAccount, m_pApp->m_TDAccount.c_str());
        strcpy(req.Password, m_pApp->m_TDPassword.c_str());
        //strcpy(req.UserProductInfo, "notthisone");
        //strcpy(req.TerminalInfo, "PC;IIP=123.112.154.118;IPORT=50361;LIP=192.168.118.107;MAC=54EE750B1713FCF8AE5CBD58;HD=TF655AY91GHRVL;@notthisone");
        m_pApi->ReqUserLogin(&req, ++m_reqID);
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

        memcpy(&m_loginField, pRspUserLoginField, sizeof(m_loginField));

        CTORATstpQryShareholderAccountField Req = {0};
        m_pApi->ReqQryShareholderAccount(&Req, ++m_reqID);
    }

    void TDImpl::OnRspQryShareholderAccount(CTORATstpShareholderAccountField *pShareholderAccountField, CTORATstpRspInfoField *pRspInfoField, int nRequestID, bool bIsLast) {
        if (pShareholderAccountField) {
            CTORATstpShareholderAccountField accountField = {0};
            memcpy(&accountField, pShareholderAccountField, sizeof(accountField));
            m_shareHolder[pShareholderAccountField->ExchangeID] = accountField;
        }

        if (bIsLast) {
            CTORATstpQrySecurityField Req = {0};
            m_pApi->ReqQrySecurity(&Req, ++m_reqID);
        }
    }

    void TDImpl::OnRspQrySecurity(CTORATstpSecurityField *pSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (pSecurity) {
            auto iter = m_marketSecurity.find(pSecurity->SecurityID);
            if (iter == m_marketSecurity.end()) {
                auto security = m_pool.Malloc<stSecurity>(sizeof(stSecurity));
                if (security) {
                    strcpy(security->SecurityID, pSecurity->SecurityID);
                    strcpy(security->SecurityName, pSecurity->SecurityName);
                    security->ExchangeID = pSecurity->ExchangeID;
                    security->UpperLimitPrice = pSecurity->UpperLimitPrice;
                    security->LowerLimitPrice = pSecurity->LowerLimitPrice;
                    m_marketSecurity[security->SecurityID] = security;

                    if (!m_pApp->m_watchSecurity.empty()) {
                        if (m_pApp->m_watchSecurity.find(security->SecurityID) != m_pApp->m_watchSecurity.end()) {
                            m_pApp->m_watchSecurity[security->SecurityID]->ExchangeID = security->ExchangeID;
                        }
                    }
                }
            }
        }

        if (bIsLast) {
            printf("TD::OnRspQrySecurity Success!!!\n");
            CTORATstpQryOrderField req = {0};
            m_pApi->ReqQryOrder(&req, 0);
        }
    }

    void TDImpl::OnRspQryOrder(CTORATstpOrderField *pOrder, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (pOrder) {
        }

        if (bIsLast) {
            printf("TD::OnRspQryOrder Success!!!\n");
            CTORATstpQryTradeField Req = {0};
            m_pApi->ReqQryTrade(&Req, 0);
        }
    }

    void TDImpl::OnRspQryTrade(CTORATstpTradeField *pTrade, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (pTrade) {
        }

        if (bIsLast) {
            printf("TD::OnRspQryTrade Success!!!\n");
            CTORATstpQryPositionField Req = {0};
            m_pApi->ReqQryPosition(&Req, 0);
        }
    }

    void TDImpl::OnRspQryPosition(CTORATstpPositionField *pPosition, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (pPosition) {
        }

        if (bIsLast) {
            printf("TD::OnRspQryPosition Success!!!\n");
            CTORATstpQryTradingAccountField Req = {0};
            m_pApi->ReqQryTradingAccount(&Req, 0);
        }
    }

    void TDImpl::OnRspQryTradingAccount(CTORATstpTradingAccountField *pTradingAccount, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (pTradingAccount) {
        }

        if (bIsLast) {
            printf("TD::OnRspQryTradingAccount Success!!!\n");
            m_isInited = true;
            m_pApp->m_ioc.post(boost::bind(&CApplication::TDOnInited, m_pApp));
        }
    }

    int TDImpl::OrderInsert(TTORATstpSecurityIDType SecurityID, TTORATstpExchangeIDType ExchangeID, TTORATstpDirectionType Direction,
                            TTORATstpVolumeType VolumeTotalOriginal, TTORATstpPriceType LimitPric) {
        CTORATstpInputOrderField req = {0};

        strcpy(req.SecurityID, SecurityID);
        req.ExchangeID = ExchangeID;
        strcpy(req.ShareholderID, m_shareHolder[ExchangeID].ShareholderID);
        req.Direction = Direction;
        req.VolumeTotalOriginal = VolumeTotalOriginal;
        req.LimitPrice = LimitPric;
        req.OrderPriceType = TORA_TSTP_OPT_LimitPrice;
        req.TimeCondition = TORA_TSTP_TC_GFD;
        req.VolumeCondition = TORA_TSTP_VC_AV;
        req.OrderRef = m_reqID;
        return m_pApi->ReqOrderInsert(&req, ++m_reqID);
    }

    void TDImpl::OnRspOrderInsert(CTORATstpInputOrderField *pInputOrderField, CTORATstpRspInfoField *pRspInfoField, int nRequestID) {
    }

    void TDImpl::OnErrRtnOrderInsert(CTORATstpInputOrderField *pInputOrderField, CTORATstpRspInfoField *pRspInfoField, int nRequestID) {
    }

    void TDImpl::OnRtnOrder(CTORATstpOrderField *pOrder) {
    }

    void TDImpl::OnRtnTrade(CTORATstpTradeField *pTrade) {
    }

    int TDImpl::OrderCancel() {
        CTORATstpInputOrderActionField req = {0};
        req.ActionFlag = TORA_TSTP_AF_Delete;
        return m_pApi->ReqOrderAction(&req, ++m_reqID);
    }

    void TDImpl::OnRspOrderAction(CTORATstpInputOrderActionField *pInputOrderActionField, CTORATstpRspInfoField *pRspInfoField, int nRequestID) {
    }

    void TDImpl::OnErrRtnOrderAction(CTORATstpInputOrderActionField *pInputOrderActionField, CTORATstpRspInfoField *pRspInfoField, int nRequestID) {
    }

}
