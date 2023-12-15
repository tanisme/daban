#ifndef TEST_BALIBALI_H
#define TEST_BALIBALI_H

#include <string>
#include <unordered_map>
#include <set>
#include <boost/asio.hpp>

#include "MDL2Impl.h"
#include "TDImpl.h"
#include "MemoryPool.h"
#include "Imitate.h"

class CApplication {
public:
    explicit CApplication(boost::asio::io_context& ioc);
    ~CApplication();

    void Init(std::string& watchSecurity);
    void Start();
    void OnTime(const boost::system::error_code& error);

    // TD
    void TDOnRspQrySecurity(PROTD::CTORATstpSecurityField& Security);
    void TDOnInitFinished();

    // MD
    void MDOnInitFinished(PROMD::TTORATstpExchangeIDType exchangeID);
    void MDOnRtnOrderDetail(PROMD::CTORATstpLev2OrderDetailField& OrderDetail);
    void MDOnRtnTransaction(PROMD::CTORATstpLev2TransactionField& Transaction);
    void MDOnRtnNGTSTick(PROMD::CTORATstpLev2NGTSTickField& Tick);
    void InsertOrder(PROMD::TTORATstpSecurityIDType SecurityID, PROMD::TTORATstpLongSequenceType OrderNO, PROMD::TTORATstpPriceType Price, PROMD::TTORATstpLongVolumeType Volume, PROMD::TTORATstpLSideType Side);
    void ModifyOrder(PROMD::TTORATstpSecurityIDType SecurityID, PROMD::TTORATstpLongVolumeType Volume, PROMD::TTORATstpLongSequenceType OrderNo, PROMD::TTORATstpTradeBSFlagType Side);
    void ShowOrderBook(PROMD::TTORATstpSecurityIDType SecurityID);
    double GetOrderNoPrice(int SecurityIDInt, PROMD::TTORATstpLongSequenceType OrderNO);
    void AddOrderNoPrice(int SecurityIDInt, PROMD::TTORATstpLongSequenceType OrderNO, PROMD::TTORATstpPriceType Price);
    void DelOrderNoPrice(int SecurityIDInt, PROMD::TTORATstpLongSequenceType OrderNO);
    void AddHomebestOrder(PROMD::TTORATstpSecurityIDType SecurityID, PROMD::TTORATstpLongSequenceType OrderNO, PROMD::TTORATstpLongVolumeType Volume, PROMD::TTORATstpLSideType Side);
    stHomebestOrder* GetHomebestOrder(PROMD::TTORATstpSecurityIDType SecurityID, PROMD::TTORATstpLongSequenceType OrderNO);
    void DelHomebestOrder(PROMD::TTORATstpSecurityIDType SecurityID, PROMD::TTORATstpLongSequenceType OrderNO);

public:
    boost::asio::io_context& m_ioc;
    boost::asio::deadline_timer m_timer;

public:
    bool m_isTest = true;
    bool m_isSHExchange = true;
    bool m_isSHNewversion = false;
    std::string m_TDAddr = "tcp://210.14.72.21:4400";
    std::string m_TDAccount = "00030557";
    std::string m_TDPassword = "17522830";
    std::string m_MDAddr = "tcp://210.14.72.17:16900";
    std::string m_MDInterface = "tcp://210.14.72.17:16900";
    std::string m_dataDir = "";

    MemoryPool m_pool;
    std::unordered_map<std::string, stSecurity*> m_marketSecurity;
    std::unordered_map<std::string, stSecurity*> m_watchSecurity;

private:
    PROTD::TDImpl *m_TD = nullptr;
    PROMD::MDL2Impl *m_MD = nullptr;
    MapOrderV m_orderBuyV;
    MapOrderV m_orderSellV;

    std::unordered_map<int, std::unordered_map<PROMD::TTORATstpLongSequenceType,PROMD::TTORATstpPriceType>> m_orderNoPrice;
    std::unordered_map<std::string, std::unordered_map<PROMD::TTORATstpLongSequenceType, stHomebestOrder*>> m_homeBaseOrder;

    test::Imitate m_imitate;
};

#endif //TEST_BALIBALI_H
