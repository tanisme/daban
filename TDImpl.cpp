#include "TDImpl.h"
#include "CApplication.h"
#include "LocalConfig.h"

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
        if (m_tdApi == nullptr) return false;

        m_tdApi->RegisterSpi(this);
        m_tdApi->RegisterFront((char *) LocalConfig::GetMe().GetTDAddr().c_str());
        m_tdApi->SubscribePrivateTopic(TORA_TERT_QUICK);
        m_tdApi->SubscribePublicTopic(TORA_TERT_QUICK);
        m_tdApi->Init();
        return true;
    }

    void TDImpl::OnFrontConnected() {
        printf("交易连接成功!!!\n");

        CTORATstpReqUserLoginField req = {0};
        req.LogInAccountType = TORA_TSTP_LACT_AccountID;
        strcpy(req.LogInAccount, LocalConfig::GetMe().GetTDAccount().c_str());
        strcpy(req.Password, LocalConfig::GetMe().GetTDPassword().c_str());
        strcpy(req.UserProductInfo, "notthisone");
        strcpy(req.TerminalInfo,
               "PC;IIP=123.112.154.118;IPORT=50361;LIP=192.168.118.107;MAC=54EE750B1713FCF8AE5CBD58;HD=TF655AY91GHRVL;@notthisone");
        m_tdApi->ReqUserLogin(&req, ++m_reqID);
    }

    void TDImpl::OnFrontDisconnected(int nReason) {
        printf("交易断开连接!!! 原因:%d\n", nReason);
        m_isLogined = false;
    }

    void TDImpl::OnRspUserLogin(CTORATstpRspUserLoginField *pRspUserLoginField, CTORATstpRspInfoField *pRspInfo,
                                int nRequestID) {
        if (!pRspUserLoginField || !pRspInfo) return;

        if (pRspInfo->ErrorID > 0) {
            printf("交易登陆失败!!! 错误信息:%s\n", pRspInfo->ErrorMsg);
            return;
        }

        m_isLogined = true;
        m_pApp->TDOnRspUserLogin(pRspUserLoginField);
        printf("交易登陆成功!!!\n");

        CTORATstpQryShareholderAccountField req = {0};
        m_tdApi->ReqQryShareholderAccount(&req, ++m_reqID);
    }

    void TDImpl::OnRspQryShareholderAccount(CTORATstpShareholderAccountField *pShareholderAccount,
                                            CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (pShareholderAccount) {
            m_pApp->TDOnRspQryShareholderAccount(pShareholderAccount);
        }

        if (bIsLast) {
            printf("交易 查询股东账户结束 开始查询合约!!!\n");
            CTORATstpQrySecurityField Req = {0};
            m_tdApi->ReqQrySecurity(&Req, ++m_reqID);
        }
    }

    void TDImpl::OnRspQrySecurity(CTORATstpSecurityField *pSecurity, CTORATstpRspInfoField *pRspInfo,
                                  int nRequestID, bool bIsLast) {
        if (pSecurity) {
            m_pApp->TDOnRspQrySecurity(pSecurity);
        }

        if (bIsLast) {
            printf("交易 查询合约结束 开始查询委托单!!!\n");
            CTORATstpQryOrderField req = {0};
            m_tdApi->ReqQryOrder(&req, 0);
        }
    }

    void TDImpl::OnRspQryOrder(CTORATstpOrderField *pOrder, CTORATstpRspInfoField *pRspInfo,
                               int nRequestID, bool bIsLast) {
        if (pOrder) {
            m_pApp->TDOnRspQryOrder(pOrder);
        }

        if (bIsLast) {
            printf("交易 查询委托单结束 开始查询成交单!!!\n");
            CTORATstpQryTradeField Req = {0};
            m_tdApi->ReqQryTrade(&Req, 0);
        }
    }

    void TDImpl::OnRspQryTrade(CTORATstpTradeField *pTrade, CTORATstpRspInfoField *pRspInfo,
                               int nRequestID, bool bIsLast) {
        if (pTrade) {
            m_pApp->TDOnRspQryTrade(pTrade);
        }

        if (bIsLast) {
            printf("交易 查询成交单结束 开始查询持仓!!!\n");
            CTORATstpQryPositionField Req = {0};
            m_tdApi->ReqQryPosition(&Req, 0);
        }
    }

    void TDImpl::OnRspQryPosition(CTORATstpPositionField *pPosition, CTORATstpRspInfoField *pRspInfo,
                                  int nRequestID, bool bIsLast) {
        if (pPosition) {
            m_pApp->TDOnRspQryPosition(pPosition);
        }

        if (bIsLast) {
            printf("交易 查询持仓结束 开始查询资金!!!\n");
            CTORATstpQryTradingAccountField Req = {0};
            m_tdApi->ReqQryTradingAccount(&Req, 0);
        }
    }

    void TDImpl::OnRspQryTradingAccount(CTORATstpTradingAccountField *pTradingAccount, CTORATstpRspInfoField *pRspInfo,
                                        int nRequestID, bool bIsLast) {
        if (pTradingAccount) {
            m_pApp->TDOnRspQryTradingAccount(pTradingAccount);
        }

        if (bIsLast) {
            printf("交易 查询资金结束 交易查询完成!!!\n");
        }
    }

    int TDImpl::OrderInsert(TTORATstpSecurityIDType SecurityID, TTORATstpDirectionType Direction,
                            TTORATstpVolumeType VolumeTotalOriginal, TTORATstpPriceType LimitPric) {
        CTORATstpInputOrderField req = {0};
        //req.ExchangeID = TORA_TSTP_EXD_SSE;
        //strcpy(req.ShareholderID, SH_ShareHolderID);
        //strcpy(req.SecurityID, "600000");
        //req.Direction = TORA_TSTP_D_Buy;
        //req.VolumeTotalOriginal = 200;

        // ????????????????????嵵??????????嵵?????????м???????????????????????????????????????м??????????????????
        // ???????????????????????????????????????????????????????????????嵵????????м????
        // ??????????????????????????????????д????????????м??????????д???????
        // ?????????????????????????????ο????????????????дOrderPriceType??TimeCondition??VolumeCondition???????:
        //req.LimitPrice = 7.00;
        //req.OrderPriceType = TORA_TSTP_OPT_LimitPrice;
        //req.TimeCondition = TORA_TSTP_TC_GFD;
        //req.VolumeCondition = TORA_TSTP_VC_AV;

        // OrderRef????????????????????????α????????
        // ??????д?????????????????????????????????
        // ????д?????豣?????TCP???±???????????????????????????????????????1??????
        //req.OrderRef = 1;
        //InvestorID????????д???豣???д???
        //Operway???з????????????????д?????????????????

        // ??????????Σ?????????????д??????ε????????????????????????????????????????????????
        //strcpy(req.SInfo, "sinfo");
        //req.IInfo = 678;

        //??????????

        strcpy(req.SecurityID, SecurityID);
        req.Direction = Direction;
        req.VolumeTotalOriginal = VolumeTotalOriginal;
        req.LimitPrice = LimitPric;
        return m_tdApi->ReqOrderInsert(&req, ++m_reqID);
    }

    void TDImpl::OnRspOrderInsert(CTORATstpInputOrderField *pInputOrderField, CTORATstpRspInfoField *pRspInfoField,
                                  int nRequestID) {
        m_pApp->TDOnRspOrderInsert(pInputOrderField);
    }

    void TDImpl::OnErrRtnOrderInsert(CTORATstpInputOrderField *pInputOrderField, CTORATstpRspInfoField *pRspInfoField,
                                     int nRequestID) {
        m_pApp->TDOnErrRtnOrderInsert(pInputOrderField);
    }

    void TDImpl::OnRtnOrder(CTORATstpOrderField *pOrder) {
        m_pApp->TDOnRtnOrder(pOrder);
    }

    void TDImpl::OnRtnTrade(CTORATstpTradeField *pTrade) {
        m_pApp->TDOnRtnTrade(pTrade);
    }

    int TDImpl::OrderCancel() {
        CTORATstpInputOrderActionField req = {0};
        //req.ExchangeID = TORA_TSTP_EXD_SSE;
        //req.ActionFlag = TORA_TSTP_AF_Delete;
        // ???????????????????λ????????
        // ??1?????????÷??
        //req.OrderRef = 1;
        //req.FrontID = m_front_id;
        //req.SessionID = m_session_id;
        // ??2?????????????
        //strcpy(req.OrderSysID, "110019400000006");
        // OrderActionRef??????????????÷??????????????????????

        // ??????????Σ?????????????д??????ε????????????????????????????????????????
        //strcpy(req.SInfo, "sinfo");
        //req.IInfo = 678;

        // ??з????θ???????????д?????????????????
        //Operway
        return m_tdApi->ReqOrderAction(&req, ++m_reqID);
    }

    void TDImpl::OnRspOrderAction(CTORATstpInputOrderActionField *pInputOrderActionField,
                                  CTORATstpRspInfoField *pRspInfoField, int nRequestID) {
        m_pApp->TDOnRspOrderAction(pInputOrderActionField);
    }

    void TDImpl::OnErrRtnOrderAction(CTORATstpInputOrderActionField *pInputOrderActionField,
                                     CTORATstpRspInfoField *pRspInfoField, int nRequestID) {
        m_pApp->TDOnErrRtnOrderAction(pInputOrderActionField);
    }

}
