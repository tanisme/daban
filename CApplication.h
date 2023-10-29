#ifndef TEST_BALIBALI_H
#define TEST_BALIBALI_H

#include <string>
#include <unordered_map>
#include <boost/asio.hpp>

#include "sqliteimpl.h"
#include "MDL2Impl.h"
#include "TDImpl.h"

class CApplication {
public:
    explicit CApplication(boost::asio::io_context& ioc);
    ~CApplication();

    void Start();
    void OnTime(const boost::system::error_code& error);
    void OnHttpHandle(int serviceNo);

    // MD
    void MDOnRspUserLogin(PROMD::TTORATstpExchangeIDType exchangeID);
    void MDPostPrice(stPostPrice& postPrice);

    // TD
    void TDOnRspUserLogin(PROTD::CTORATstpRspUserLoginField& pRspUserLoginField);
    void TDOnRspQrySecurity(PROTD::CTORATstpSecurityField& pSecurity);
    void TDOnRspQryOrder(PROTD::CTORATstpOrderField& pOrder);
    void TDOnRspQryTrade(PROTD::CTORATstpTradeField& pTrade);
    void TDOnRspQryPosition(PROTD::CTORATstpPositionField& pPosition);
    void TDOnRspQryTradingAccount(PROTD::CTORATstpTradingAccountField& pTradingAccount);
    void TDOnRspOrderInsert(PROTD::CTORATstpInputOrderField& pInputOrderField);
    void TDOnErrRtnOrderInsert(PROTD::CTORATstpInputOrderField& pInputOrderField);
    void TDOnRtnOrder(PROTD::CTORATstpOrderField& pOrder);
    void TDOnRtnTrade(PROTD::CTORATstpTradeField& pTrade);
    void TDOnRspOrderAction(PROTD::CTORATstpInputOrderActionField& pInputOrderActionField);
    void TDOnErrRtnOrderAction(PROTD::CTORATstpInputOrderActionField& pInputOrderActionField);

public:
    boost::asio::io_context& m_ioc;
    boost::asio::deadline_timer m_timer;

public:
    bool Init();
    bool LoadSubSecurityIDs();
    bool AddSubSecurity(char *SecurityID, char ExchangeID);
    bool DelSubSecurity(char *SecurityID);
    bool LoadStrategy();
    bool AddStrategy(stStrategy & strategy);
    std::string GetDBFile() const { return m_dbFile; }
    void SetDBFile(std::string &dbFile) { m_dbFile = dbFile; }
    std::string GetSHMDAddr() const { return m_shMDAddr; }
    void SetSHMDAddr(std::string &addr) { m_shMDAddr = addr; }
    std::string GetSZMDAddr() const { return m_szMDAddr; }
    void SetSZMDAddr(std::string &addr) { m_szMDAddr = addr; }
    std::string GetTDAddr() const { return m_TDAddr; }
    void SetTDAddr(std::string &addr) { m_TDAddr = addr; }
    std::string GetTDAccount() const { return m_TDAccount; }
    void SetTDAccount(std::string &account) { m_TDAccount = account; }
    std::string GetTDPassword() const { return m_TDPassword; }
    void SetTDPassword(std::string &password) { m_TDPassword = password; }
    bool GetSHMDIsNew() const { return m_shMDNewVersion > 0; }
    void SetSHMdIsNew(int isNew) { m_shMDNewVersion = isNew; }

    std::unordered_map<std::string, stSubSecurity> m_subSecurityIDs;
    std::unordered_map<std::string, std::vector<stStrategy> > m_strategys;
private:
    PROMD::MDL2Impl *GetMDByExchangeID(PROMD::TTORATstpExchangeIDType ExchangeID);

private:
    std::string m_dbFile;
    std::string m_shMDAddr = "tcp://210.14.72.17:16900";
    std::string m_szMDAddr = "tcp://210.14.72.17:6900";
    std::string m_TDAddr = "tcp://210.14.72.21:4400";
    std::string m_TDAccount = "00030557";
    std::string m_TDPassword = "17522830";
    int m_shMDNewVersion = 0;
    PROMD::MDL2Impl *m_shMD = nullptr;
    PROMD::MDL2Impl *m_szMD = nullptr;
    PROTD::TDImpl *m_TD = nullptr;
    SQLite3::ptr m_db = nullptr;

    std::unordered_map<std::string, PROTD::CTORATstpSecurityField *> m_securityIDs;
    std::unordered_map<std::string, PROTD::CTORATstpOrderField *> m_order;
    std::unordered_map<std::string, PROTD::CTORATstpPositionField *> m_position;
};

#endif //TEST_BALIBALI_H
