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

    for (auto iter = m_security.begin(); iter != m_security.end(); ++iter) {
        auto ptr = iter->second;
        m_security.erase(iter);
        free(ptr);
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

    m_shMD->ShowOrderBook();
    m_szMD->ShowOrderBook();
    m_timer.expires_from_now(boost::posix_time::milliseconds(1000));
    m_timer.async_wait(boost::bind(&CApplication::OnTime, this, boost::asio::placeholders::error));
}

void CApplication::OnFileHandle(int ServiceNo) {
    printf("Onfilehandle\n");
}

/***************************************MD***************************************/
void CApplication::MDOnRspUserLogin(PROMD::TTORATstpExchangeIDType exchangeID) {
    auto md = GetMDByExchangeID(exchangeID);
    if (!md) return;
    for (auto &iter: LocalConfig::GetMe().m_mapSecurityID) {
        if (exchangeID == iter.second.ExchangeID && iter.second.Status == 0) {
            PROMD::TTORATstpSecurityIDType Security;
            strncpy(Security, iter.first.c_str(), sizeof(Security));
            //md->ReqMarketData(Security, exchangeID, 1);
            if (exchangeID == PROMD::TORA_TSTP_EXD_SSE) {
                if (LocalConfig::GetMe().GetSHMDIsNew()) {
                    md->ReqMarketData(Security, exchangeID, 4);
                } else {
                    md->ReqMarketData(Security, exchangeID, 2);
                    md->ReqMarketData(Security, exchangeID, 3);
                }
            } else if (exchangeID == PROMD::TORA_TSTP_EXD_SZSE) {
                md->ReqMarketData(Security, exchangeID, 2);
                md->ReqMarketData(Security, exchangeID, 3);
            }
        }
    }
}


/***************************************TD***************************************/
void CApplication::TDOnRspUserLogin(PROTD::CTORATstpRspUserLoginField& RspUserLoginField) {
    printf("CApplication::TDOnRspUserLogin Success!!!\n");
}

void CApplication::TDOnRspQrySecurity(PROTD::CTORATstpSecurityField& Security) {
    //printf("SecurityID:%s,SecurityName:%s,UpperLimitPrice:%lf,LowerLimitPrice:%lf\n", Security.SecurityID, Security.SecurityName, Security.UpperLimitPrice, Security.LowerLimitPrice);
    auto security = (PROTD::CTORATstpSecurityField *) malloc(sizeof(PROTD::CTORATstpSecurityField));
    if (security) {
        memcpy(security, &Security, sizeof(PROTD::CTORATstpSecurityField));

        if (m_security.find(Security.SecurityID) != m_security.end()) {
            PROTD::CTORATstpSecurityField *old = m_security[security->SecurityID];
            m_security.erase(security->SecurityID);
            free(old);
        }
        m_security[security->SecurityID] = security;
    }
}

void CApplication::TDOnRspQryOrder(PROTD::CTORATstpOrderField& Order) {
    printf("OrderLocalID:%s,SecurityID:%s,Direction:%c,VolumeTotalOriginal:%d,LimitPrice:%lf,VolumeTraded:%d,VolumeCanceled:%d,OrderSysID:%s,FrontID:%d,SessionID:%d,OrderRef:%d,OrderStatus:%c,StatusMsg:%s,InsertTime:%s\n",
           Order.OrderLocalID, Order.SecurityID, Order.Direction, Order.VolumeTotalOriginal, Order.LimitPrice,
           Order.VolumeTraded, Order.VolumeCanceled, Order.OrderSysID, Order.FrontID, Order.SessionID,
           Order.OrderRef, Order.OrderStatus, Order.StatusMsg, Order.InsertTime);
}

void CApplication::TDOnRspQryTrade(PROTD::CTORATstpTradeField& Trade) {
    printf("OrderLocalID:%s,SecurityID:%s,Direction:%c,Volume:%d,Price:%lf,OrderSysID:%s,OrderRef:%d,TradeTime:%s\n",
           Trade.OrderLocalID, Trade.SecurityID, Trade.Direction, Trade.Volume, Trade.Price,
           Trade.OrderSysID, Trade.OrderRef, Trade.TradeTime);
}

void CApplication::TDOnRspQryPosition(PROTD::CTORATstpPositionField& Position) {
    printf("SecurityID:%s,CurrentPosition:%d,OpenPosCost:%lf,TotalPosCost:%lf,PrePosition:%d,HistoryPosFrozen:%d\n",
           Position.SecurityID, Position.CurrentPosition, Position.OpenPosCost, Position.TotalPosCost,
           Position.PrePosition, Position.HistoryPosFrozen);
    //auto pos = (PROTD::CTORATstpPositionField *) malloc(sizeof(PROTD::CTORATstpPositionField));
    //if (pos) {
    //    memcpy(pos, pPosition, sizeof(*pos));

    //}
}

void CApplication::TDOnRspQryTradingAccount(PROTD::CTORATstpTradingAccountField& TradingAccount) {
    printf("AccountID:%s,UsefulMoney:%lf,FrozenCash:%lf,FrozenCommission:%lf\n",
           TradingAccount.AccountID, TradingAccount.UsefulMoney, TradingAccount.FrozenCash, TradingAccount.FrozenCommission);
}

void CApplication::TDOnRspOrderInsert(PROTD::CTORATstpInputOrderField& pInputOrderField) {

}

void CApplication::TDOnErrRtnOrderInsert(PROTD::CTORATstpInputOrderField& pInputOrderField) {

}

void CApplication::TDOnRtnOrder(PROTD::CTORATstpOrderField& pOrder) {

}

void CApplication::TDOnRtnTrade(PROTD::CTORATstpTradeField& pTrade) {

}

void CApplication::TDOnRspOrderAction(PROTD::CTORATstpInputOrderActionField& pInputOrderActionField) {

}

void CApplication::TDOnErrRtnOrderAction(PROTD::CTORATstpInputOrderActionField& pInputOrderActionField) {

}

PROMD::MDL2Impl *CApplication::GetMDByExchangeID(PROMD::TTORATstpExchangeIDType ExchangeID) {
    if (ExchangeID == PROMD::TORA_TSTP_EXD_SSE) return m_shMD;
    return m_szMD;
}

