#ifndef TEST_BALIBALI_H
#define TEST_BALIBALI_H

#include <string>
#include <unordered_map>
#include <boost/asio.hpp>

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
    bool m_isSHNewversion = false;
    std::string m_TDAddr = "tcp://210.14.72.21:4400";
    std::string m_TDAccount = "00030557";
    std::string m_TDPassword = "17522830";
    std::string m_shMDAddr = "tcp://210.14.72.17:16900";
    std::string m_shMDInterface = "tcp://210.14.72.17:16900";
    std::string m_szMDAddr = "tcp://210.14.72.17:16900";
    std::string m_szMDInterface = "tcp://210.14.72.17:16900";

    MemoryPool m_pool;
    std::unordered_map<PROMD::TTORATstpExchangeIDType, bool> m_supportExchangeID;
    std::unordered_map<std::string, stSecurity*> m_watchSecurity;
private:
    PROTD::TDImpl *m_TD = nullptr;
    PROMD::MDL2Impl *m_shMD = nullptr;
    PROMD::MDL2Impl *m_szMD = nullptr;
};

#endif //TEST_BALIBALI_H
