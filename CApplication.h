#ifndef TEST_BALIBALI_H
#define TEST_BALIBALI_H

#include <string>
#include <unordered_map>
#include <boost/asio.hpp>

#include "sqliteimpl.h"
#include "MDL2Impl.h"
#include "TDImpl.h"
#include "MemoryPool.h"

class CApplication {
public:
    explicit CApplication(boost::asio::io_context& ioc);
    ~CApplication();

    bool Init(std::string& watchSecurity, std::string& currentExchangeID);
    void Start();
    void OnTime(const boost::system::error_code& error);

    // MD
    void MDOnInited(PROMD::TTORATstpExchangeIDType exchangeID);
    void MDPostPrice(stPostPrice& postPrice);

    // TD
    void TDOnInited();

public:
    boost::asio::io_context& m_ioc;
    boost::asio::deadline_timer m_timer;

public:
    bool LoadStrategy();
    bool AddStrategy(stStrategy & strategy);
    bool ModStrategy(stStrategy* strategy);

public:
    bool m_isTest = true;
    bool m_isStrategyOpen = false;
    bool m_isSHMDNewVersion = true;
    std::string m_dbFile;
    std::string m_TDAddr = "tcp://210.14.72.21:4400";
    std::string m_TDAccount = "00030557";
    std::string m_TDPassword = "17522830";
    std::string m_shCpucore = "";
    std::string m_szCpucore = "";
    //ceshi
    std::string m_testSHMDAddr = "tcp://210.14.72.17:16900";
    std::string m_testSZMDAddr = "tcp://210.14.72.17:6900";
    //shipan
    std::string m_shMDAddr = "tcp://210.14.72.17:16900";
    std::string m_shMDInterface = "tcp://210.14.72.17:16900";
    std::string m_szMDAddr = "tcp://210.14.72.17:16900";
    std::string m_szMDInterface = "tcp://210.14.72.17:16900";

    MemoryPool m_pool;
    std::unordered_map<PROMD::TTORATstpExchangeIDType, bool> m_supportExchangeID;
    std::unordered_map<std::string, stSecurity*> m_watchSecurity;
    std::unordered_map<std::string, std::vector<stStrategy*> > m_strategys;

private:
    PROMD::MDL2Impl *m_shMD = nullptr;
    PROMD::MDL2Impl *m_szMD = nullptr;
    PROTD::TDImpl *m_TD = nullptr;
    SQLite3::ptr m_db = nullptr;
};

#endif //TEST_BALIBALI_H
