#ifndef TEST_BALIBALI_H
#define TEST_BALIBALI_H

#include <string>
#include <unordered_map>
#include <boost/asio.hpp>

#include "MDL2Impl.h"
#include "TDImpl.h"

class CApplication {
public:
    explicit CApplication(boost::asio::io_context& ioc);

    ~CApplication();

    void Start();
    void OnTime(const boost::system::error_code& error);
    void OnFileHandle(int ServiceNo);

    // 行情
    void MDOnRspUserLogin(PROMD::TTORATstpExchangeIDType exchangeID);

    // 交易
    void TDOnRspUserLogin(PROTD::CTORATstpRspUserLoginField *pRspUserLoginField);

    void TDOnRspQryShareholderAccount(PROTD::CTORATstpShareholderAccountField *pShareholderAccount);

    void TDOnRspQrySecurity(PROTD::CTORATstpSecurityField *pSecurity);

    void TDOnRspQryOrder(PROTD::CTORATstpOrderField *pOrder);

    void TDOnRspQryTrade(PROTD::CTORATstpTradeField *pTrade);

    void TDOnRspQryPosition(PROTD::CTORATstpPositionField *pPosition);

    void TDOnRspQryTradingAccount(PROTD::CTORATstpTradingAccountField *pTradingAccount);

    void TDOnRspOrderInsert(PROTD::CTORATstpInputOrderField *pInputOrderField);

    void TDOnErrRtnOrderInsert(PROTD::CTORATstpInputOrderField *pInputOrderField);

    void TDOnRtnOrder(PROTD::CTORATstpOrderField *pOrder);

    void TDOnRtnTrade(PROTD::CTORATstpTradeField *pTrade);

    void TDOnRspOrderAction(PROTD::CTORATstpInputOrderActionField *pInputOrderActionField);

    void TDOnErrRtnOrderAction(PROTD::CTORATstpInputOrderActionField *pInputOrderActionField);

    boost::asio::io_context& m_ioc;

    boost::asio::deadline_timer m_timer;
private:
    PROMD::MDL2Impl *GetMDByExchangeID(PROMD::TTORATstpExchangeIDType ExchangeID);

private:
    bool m_shMD_open = true;
    bool m_szMD_open = true;
    bool m_td_open = true;
    PROMD::MDL2Impl *m_shMD = nullptr;
    PROMD::MDL2Impl *m_szMD = nullptr;
    PROTD::TDImpl *m_TD = nullptr;

    std::unordered_map<std::string, PROTD::CTORATstpSecurityField *> m_security;
    std::unordered_map<std::string, PROTD::CTORATstpOrderField *> m_order;
    std::unordered_map<std::string, PROTD::CTORATstpPositionField *> m_position;
};

#endif //TEST_BALIBALI_H
