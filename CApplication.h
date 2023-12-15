﻿#ifndef TEST_BALIBALI_H
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
    void ShowOrderBook(PROMD::TTORATstpSecurityIDType SecurityID);
    double GetOrderNoPrice(int SecurityIDInt, PROMD::TTORATstpLongSequenceType OrderNO);
    void AddOrderNoPrice(int SecurityIDInt, PROMD::TTORATstpLongSequenceType OrderNO, PROMD::TTORATstpPriceType Price);
    void DelOrderNoPrice(int SecurityIDInt, PROMD::TTORATstpLongSequenceType OrderNO);
    void AddHomebestOrder(PROMD::TTORATstpSecurityIDType SecurityID, PROMD::TTORATstpLongSequenceType OrderNO, PROMD::TTORATstpPriceType Price, PROMD::TTORATstpLongVolumeType Volume, PROMD::TTORATstpLSideType Side, PROMD::TTORATstpExchangeIDType ExchangeID);
    stHomebestOrder* GetHomebestOrder(PROMD::TTORATstpSecurityIDType SecurityID, PROMD::TTORATstpLongSequenceType OrderNO);

    void InitOrderMapN(bool isTest = false);
    int GetPriceIndexN(PROMD::TTORATstpSecurityIDType SecurityID, PROMD::TTORATstpPriceType Price, PROMD::TTORATstpTradeBSFlagType Side);
    void InsertOrderN(PROMD::TTORATstpSecurityIDType SecurityID, PROMD::TTORATstpLongSequenceType OrderNO, PROMD::TTORATstpPriceType Price, PROMD::TTORATstpLongVolumeType Volume, PROMD::TTORATstpLSideType Side);
    double ModifyOrderN(PROMD::TTORATstpSecurityIDType SecurityID, PROMD::TTORATstpLongVolumeType Volume, PROMD::TTORATstpLongSequenceType OrderNo, PROMD::TTORATstpTradeBSFlagType Side);
    void FixOrderN(PROMD::TTORATstpSecurityIDType SecurityID, PROMD::TTORATstpLongSequenceType OrderNO, PROMD::TTORATstpPriceType Price, PROMD::TTORATstpLongVolumeType Volume, PROMD::TTORATstpLSideType Side);
    void AddOrderIndexN(int SecutityIDInt, int idx, PROMD::TTORATstpTradeBSFlagType Side);
    void ModOrderIndexN(int SecurityIDInt, int idx, PROMD::TTORATstpTradeBSFlagType Side);

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

    std::unordered_map<int, std::unordered_map<PROMD::TTORATstpLongSequenceType,PROMD::TTORATstpPriceType>> m_orderNoPrice;
    std::unordered_map<std::string, std::unordered_map<PROMD::TTORATstpLongSequenceType, stHomebestOrder*>> m_homeBaseOrder;

    MapOrderN m_orderBuyN;
    MapOrderN m_orderSellN;
    std::unordered_map<int, std::unordered_map<int, int>> m_orderBuyNIndex;
    std::unordered_map<int, std::unordered_map<int, int>> m_orderSellNIndex;

    test::Imitate m_imitate;
};

#endif //TEST_BALIBALI_H
