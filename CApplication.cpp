#include "CApplication.h"
#include "LocalConfig.h"
#include <iostream>
#include <boost/bind/bind.hpp>

CApplication::CApplication(boost::asio::io_context& ioc)
        : m_ioc(ioc)
        , m_timer(m_ioc, boost::posix_time::milliseconds(1000)) {
}

CApplication::~CApplication() {
    if (m_shMD != nullptr) {
        m_shMD->GetApi()->Join();
        m_shMD->GetApi()->Release();
        delete m_shMD;
    }
    if (m_szMD != nullptr) {
        m_szMD->GetApi()->Join();
        m_szMD->GetApi()->Release();
        delete m_szMD;
    }

    if (m_TD != nullptr) {
        m_TD->GetApi()->Join();
        m_TD->GetApi()->Release();
        delete m_TD;
    }
}

void CApplication::Start() {
    m_shMD = new PROMD::MDL2Impl(this, PROMD::TORA_TSTP_EXD_SSE);
    m_shMD->Start();

    m_szMD = new PROMD::MDL2Impl(this, PROMD::TORA_TSTP_EXD_SZSE);
    m_szMD->Start();

    m_TD = new PROTD::TDImpl(this);
    m_TD->Start();

    m_timer.expires_from_now(boost::posix_time::milliseconds(1000));
    m_timer.async_wait(boost::bind(&CApplication::OnTime, this, boost::asio::placeholders::error));
}

void CApplication::OnTime(const boost::system::error_code& error)
{
    if (error) {
        if (error == boost::asio::error::operation_aborted) {
            return;
        }
    }

    std::cout << "hello" << std::endl;
    m_timer.expires_from_now(boost::posix_time::milliseconds(1000));
    m_timer.async_wait(boost::bind(&CApplication::OnTime, this, boost::asio::placeholders::error));
}

/***************************************MD***************************************/
void CApplication::MDOnRspUserLogin(PROMD::TTORATstpExchangeIDType exchangeID) {
    PROMD::MDL2Impl *md = GetMDByExchangeID(exchangeID);
    if (md == nullptr) return;
    for (auto &iter: LocalConfig::GetMe().m_mapSecurityID) {
        if (exchangeID == iter.second.ExchangeID) {
            PROMD::TTORATstpSecurityIDType Security;
            strncpy(Security, iter.first.c_str(), sizeof(Security));
            //md->ReqMarketData(Security, exchangeID, 1);
            md->ReqMarketData(Security, exchangeID, 2);
            md->ReqMarketData(Security, exchangeID, 3);
        }
    }
}


/***************************************TD***************************************/
void CApplication::TDOnRspUserLogin(PROTD::CTORATstpRspUserLoginField *pRspUserLoginField) {
    printf("TD::OnRspUserLogin success!!!\n");
}

void CApplication::TDOnRspQryShareholderAccount(PROTD::CTORATstpShareholderAccountField *pShareholderAccount) {
    switch (pShareholderAccount->ExchangeID) {
        case PROTD::TORA_TSTP_EXD_SSE:
            printf("SSE.ShareholderID=%s\n", pShareholderAccount->ShareholderID);
            break;
        case PROTD::TORA_TSTP_EXD_SZSE:
            printf("SZSE.ShareholderID=%s\n", pShareholderAccount->ShareholderID);
            break;
        case PROTD::TORA_TSTP_EXD_BSE:
            printf("BSE.ShareholderID=%s\n", pShareholderAccount->ShareholderID);
            break;
        case PROTD::TORA_TSTP_EXD_HK:
            printf("HK.ShareholderID=%s\n", pShareholderAccount->ShareholderID);
            break;
        default:
            break;
    }
}

void CApplication::TDOnRspQrySecurity(PROTD::CTORATstpSecurityField *pSecurity) {
    if (pSecurity) {
        //printf("SecurityID:%s,SecurityName:%s,PriceTick:%lf,PreClosePrice:%lf,MarketID:%c\n", pSecurity->SecurityID, pSecurity->SecurityName, pSecurity->PriceTick, pSecurity->PreClosePrice, pSecurity->MarketID);
        auto security = (PROTD::CTORATstpSecurityField *) malloc(sizeof(PROTD::CTORATstpSecurityField));
        if (security) {
            memcpy(security, pSecurity, sizeof(PROTD::CTORATstpSecurityField));
            m_security[security->SecurityID] = security;
        }
    }
}

void CApplication::TDOnRspQryOrder(PROTD::CTORATstpOrderField *pOrder) {
    printf("OrderLocalID:%s,SecurityID:%s,Direction:%c,VolumeTotalOriginal:%d,LimitPrice:%lf,VolumeTraded:%d,VolumeCanceled:%d,OrderSysID:%s,FrontID:%d,SessionID:%d,OrderRef:%d,OrderStatus:%c,StatusMsg:%s,InsertTime:%s\n",
           pOrder->OrderLocalID, pOrder->SecurityID, pOrder->Direction, pOrder->VolumeTotalOriginal, pOrder->LimitPrice,
           pOrder->VolumeTraded, pOrder->VolumeCanceled, pOrder->OrderSysID, pOrder->FrontID, pOrder->SessionID,
           pOrder->OrderRef, pOrder->OrderStatus, pOrder->StatusMsg, pOrder->InsertTime);
}

void CApplication::TDOnRspQryTrade(PROTD::CTORATstpTradeField *pTrade) {
    printf("OrderLocalID:%s,SecurityID:%s,Direction:%c,Volume:%d,Price:%lf,OrderSysID:%s,OrderRef:%d,TradeTime:%s\n",
           pTrade->OrderLocalID, pTrade->SecurityID, pTrade->Direction, pTrade->Volume, pTrade->Price,
           pTrade->OrderSysID, pTrade->OrderRef, pTrade->TradeTime);
}

void CApplication::TDOnRspQryPosition(PROTD::CTORATstpPositionField *pPosition) {
    if (!pPosition) return;

    auto pos = (PROTD::CTORATstpPositionField *) malloc(sizeof(PROTD::CTORATstpPositionField));
    if (pos) {
        memcpy(pos, pPosition, sizeof(*pos));

    }
    printf("SecurityID:%s,CurrentPosition:%d,OpenPosCost:%lf,TotalPosCost:%lf,PrePosition:%d,HistoryPosFrozen:%d\n",
           pPosition->SecurityID, pPosition->CurrentPosition, pPosition->OpenPosCost, pPosition->TotalPosCost,
           pPosition->PrePosition, pPosition->HistoryPosFrozen);
}

void CApplication::TDOnRspQryTradingAccount(PROTD::CTORATstpTradingAccountField *pTradingAccount) {
    printf("AccountID:%s,UsefulMoney:%lf,FrozenCash:%lf,FrozenCommission:%lf\n",
           pTradingAccount->AccountID, pTradingAccount->UsefulMoney, pTradingAccount->FrozenCash,
           pTradingAccount->FrozenCommission);
}

void CApplication::TDOnRspOrderInsert(PROTD::CTORATstpInputOrderField *pInputOrderField) {

}

void CApplication::TDOnErrRtnOrderInsert(PROTD::CTORATstpInputOrderField *pInputOrderField) {

}

void CApplication::TDOnRtnOrder(PROTD::CTORATstpOrderField *pOrder) {

}

void CApplication::TDOnRtnTrade(PROTD::CTORATstpTradeField *pTrade) {

}

void CApplication::TDOnRspOrderAction(PROTD::CTORATstpInputOrderActionField *pInputOrderActionField) {

}

void CApplication::TDOnErrRtnOrderAction(PROTD::CTORATstpInputOrderActionField *pInputOrderActionField) {

}

PROMD::MDL2Impl *CApplication::GetMDByExchangeID(PROMD::TTORATstpExchangeIDType ExchangeID) {
    if (ExchangeID == PROMD::TORA_TSTP_EXD_SSE) return m_shMD;
    return m_szMD;
}

