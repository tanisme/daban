#include "MDL2Impl.h"
#include "CApplication.h"

namespace PROMD {

    MDL2Impl::MDL2Impl(CApplication *pApp, TTORATstpExchangeIDType ExchangeID)
            : m_pApp(pApp), m_exchangeID(ExchangeID) {
    }

    MDL2Impl::~MDL2Impl() {
        if (m_pApi) {
            m_pApi->Join();
            m_pApi->Release();
        }
    }

    void MDL2Impl::Clear() {
        ClearOrder(TORA_TSTP_LSD_Buy);
        ClearOrder(TORA_TSTP_LSD_Sell);
    }

    bool MDL2Impl::Start(bool isTest) {
        if (isTest) { // tcp
            m_pApi = CTORATstpLev2MdApi::CreateTstpLev2MdApi();
            m_pApi->RegisterSpi(this);
            if (m_exchangeID == TORA_TSTP_EXD_SSE) {
                m_pApi->RegisterFront((char *)m_pApp->m_shMDAddr.c_str());
            } else if (m_exchangeID == TORA_TSTP_EXD_SZSE) {
                m_pApi->RegisterFront((char *)m_pApp->m_szMDAddr.c_str());
            } else {
                return false;
            }
        } else { // udp
            m_pApi = CTORATstpLev2MdApi::CreateTstpLev2MdApi(TORA_TSTP_MST_MCAST);
            m_pApi->RegisterSpi(this);
            m_pApi->RegisterMulticast((char *)m_pApp->m_mdAddr.c_str(), (char*)m_pApp->m_mdInterface.c_str(), nullptr);
        }
        printf("MD::Start Bind cpucore %s %s %c\n", m_pApp->m_cpucore.c_str(), isTest?"tcp":"udp", m_exchangeID);
        m_pApi->Init(m_pApp->m_cpucore.c_str());
        return true;
    }

    int MDL2Impl::ReqMarketData(TTORATstpSecurityIDType SecurityID, TTORATstpExchangeIDType ExchangeID, int type, bool isSub) {
        char *security_arr[1];
        security_arr[0] = SecurityID;
        switch (type) {
            case 1:
                if (isSub) return m_pApi->SubscribeOrderDetail(security_arr, 1, ExchangeID);
                return m_pApi->UnSubscribeOrderDetail(security_arr, 1, ExchangeID);
            case 2:
                if (isSub) return m_pApi->SubscribeTransaction(security_arr, 1, ExchangeID);
                return m_pApi->UnSubscribeTransaction(security_arr, 1, ExchangeID);
            case 3:
                if (isSub) return m_pApi->SubscribeNGTSTick(security_arr, 1, ExchangeID);
                return m_pApi->UnSubscribeNGTSTick(security_arr, 1, ExchangeID);
        }
        return -1;
    }

    void MDL2Impl::ShowOrderBook(TTORATstpSecurityIDType SecurityID) {
        if (m_orderBuy.empty() && m_orderSell.empty()) return;

        std::stringstream stream;
        stream << "\n";
        stream << "--------- " << SecurityID << " " << GetTimeStr() << "---------\n";

        char buffer[512] = {0};
        int showPankouCount = 5;
        {
            auto iter = m_orderSell.find(SecurityID);
            if (iter != m_orderSell.end()) {
                auto size = (int) iter->second.size();
                if (showPankouCount < size) size = showPankouCount;
                for (auto i = size; i > 0; i--) {
                    memset(buffer, 0, sizeof(buffer));
                    TTORATstpLongVolumeType totalVolume = 0;
                    for (auto iter1: iter->second.at(i - 1).Orders) totalVolume += iter1->Volume;
                    sprintf(buffer, "S%d\t%.3f\t\t%d\t\t%lld\n", i, iter->second.at(i - 1).Price,
                            (int) iter->second.at(i - 1).Orders.size(), totalVolume);
                    stream << buffer;
                }
            }
        }

        {
            auto iter = m_orderBuy.find(SecurityID);
            if (iter != m_orderBuy.end()) {
                auto size = (int) iter->second.size();
                if (showPankouCount < size) size = showPankouCount;
                for (auto i = 1; i <= size; i++) {
                    memset(buffer, 0, sizeof(buffer));
                    TTORATstpLongVolumeType totalVolume = 0;
                    for (auto iter1: iter->second.at(i - 1).Orders) totalVolume += iter1->Volume;
                    sprintf(buffer, "B%d\t%.3f\t\t%d\t\t%lld\n", i, iter->second.at(i - 1).Price,
                            (int) iter->second.at(i - 1).Orders.size(), totalVolume);
                    stream << buffer;
                }
            }
        }
        printf("%s\n", stream.str().c_str());
    }

    const char *MDL2Impl::GetExchangeName(TTORATstpExchangeIDType ExchangeID) {
        switch (ExchangeID) {
            case TORA_TSTP_EXD_SSE:
                return "SH";
            case TORA_TSTP_EXD_SZSE:
                return "SZ";
            case TORA_TSTP_EXD_HK:
                return "HK";
            case TORA_TSTP_EXD_COMM:
                return "COM";
            default:
                break;
        }
        return "UNKNOW";
    }

    void MDL2Impl::OnFrontConnected() {
        printf("%s MD::OnFrontConnected!!!\n", GetExchangeName(m_exchangeID));
        CTORATstpReqUserLoginField req = {0};
        m_pApi->ReqUserLogin(&req, ++m_reqID);
    }

    void MDL2Impl::OnFrontDisconnected(int nReason) {
        printf("%s MD::OnFrontDisconnected!!! Reason:%d\n", GetExchangeName(m_exchangeID), nReason);
        m_isInited = false;
    }

    void MDL2Impl::OnRspUserLogin(CTORATstpRspUserLoginField *pRspUserLogin, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (!pRspUserLogin || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("%s MD::OnRspUserLogin Failed!!! ErrMsg:%s\n", GetExchangeName(m_exchangeID), pRspInfo->ErrorMsg);
            return;
        }
        m_isInited = true;
        m_pApp->m_ioc.post(boost::bind(&CApplication::MDOnInited, m_pApp, m_exchangeID));
    }

    void MDL2Impl::OnRspSubOrderDetail(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (!pSpecificSecurity || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("%s MD::OnRspSubOrderDetail Failed!!! ErrMsg:%s\n", GetExchangeName(m_exchangeID), pRspInfo->ErrorMsg);
            return;
        }
    }

    void MDL2Impl::OnRspUnSubOrderDetail(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (!pSpecificSecurity || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("%s MD::OnRspUnSubOrderDetail Failed!!! ErrMsg:%s\n", GetExchangeName(m_exchangeID), pRspInfo->ErrorMsg);
            return;
        }
    }

    void MDL2Impl::OnRspSubTransaction(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (!pSpecificSecurity || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("%s MD::OnRspSubTransaction Failed!!! ErrMsg:%s\n", GetExchangeName(m_exchangeID), pRspInfo->ErrorMsg);
            return;
        }
    }

    void MDL2Impl::OnRspUnSubTransaction(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (!pSpecificSecurity || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("%s MD::OnRspUnSubTransaction Failed!!! ErrMsg:%s\n", GetExchangeName(m_exchangeID), pRspInfo->ErrorMsg);
            return;
        }
    }

    void MDL2Impl::OnRspSubNGTSTick(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (!pSpecificSecurity || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("%s MD::OnRspSubNGTSTick Failed!!! ErrMsg:%s\n", GetExchangeName(m_exchangeID), pRspInfo->ErrorMsg);
            return;
        }
    }

    void MDL2Impl::OnRspUnSubNGTSTick(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (!pSpecificSecurity || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("%s MD::OnRspUnSubNGTSTick Failed!!! ErrMsg:%s\n", GetExchangeName(m_exchangeID), pRspInfo->ErrorMsg);
            return;
        }
    }

    void MDL2Impl::OnRtnOrderDetail(CTORATstpLev2OrderDetailField *pOrderDetail) {
        if (!pOrderDetail) return;
        OrderDetail(pOrderDetail->SecurityID, pOrderDetail->Side, pOrderDetail->OrderNO, pOrderDetail->Price, pOrderDetail->Volume, pOrderDetail->ExchangeID, pOrderDetail->OrderStatus);
    }

    void MDL2Impl::OnRtnTransaction(CTORATstpLev2TransactionField *pTransaction) {
        if (!pTransaction) return;
        Transaction(pTransaction->SecurityID, pTransaction->ExchangeID, pTransaction->TradeVolume, pTransaction->ExecType, pTransaction->BuyNo, pTransaction->SellNo, pTransaction->TradePrice, pTransaction->TradeTime);
    }

    void MDL2Impl::OnRtnNGTSTick(CTORATstpLev2NGTSTickField *pTick) {
        if (!pTick) return;
        NGTSTick(pTick->SecurityID, pTick->TickType, pTick->BuyNo, pTick->SellNo, pTick->Price, pTick->Volume, pTick->Side, pTick->TickTime);
    }

    /************************************HandleOrderBook***************************************/
    void MDL2Impl::OrderDetail(TTORATstpSecurityIDType SecurityID, TTORATstpTradeBSFlagType Side, TTORATstpLongSequenceType OrderNO,
                               TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpExchangeIDType ExchangeID,
                               TTORATstpLOrderStatusType OrderStatus) {
        if (Side != TORA_TSTP_LSD_Buy && Side != TORA_TSTP_LSD_Sell) return;
        if (ExchangeID == TORA_TSTP_EXD_SSE) {
            if (OrderStatus == TORA_TSTP_LOS_Add) {
                InsertOrder(SecurityID, OrderNO, Price, Volume, Side);
            } else if (OrderStatus == TORA_TSTP_LOS_Delete) {
                ModifyOrder(SecurityID, 0, OrderNO, Side);
            }
        } else if (ExchangeID == TORA_TSTP_EXD_SZSE) {
            InsertOrder(SecurityID, OrderNO, Price, Volume, Side);
        }
    }

    void MDL2Impl::Transaction(TTORATstpSecurityIDType SecurityID, TTORATstpExchangeIDType ExchangeID, TTORATstpLongVolumeType TradeVolume,
                               TTORATstpExecTypeType ExecType, TTORATstpLongSequenceType BuyNo, TTORATstpLongSequenceType SellNo,
                               TTORATstpPriceType TradePrice, TTORATstpTimeStampType TradeTime) {
        if (ExchangeID == TORA_TSTP_EXD_SSE) {
            ModifyOrder(SecurityID, TradeVolume, BuyNo, TORA_TSTP_LSD_Buy);
            ModifyOrder(SecurityID, TradeVolume, SellNo, TORA_TSTP_LSD_Sell);
            PostPrice(SecurityID, TradePrice);
        } else if (ExchangeID == TORA_TSTP_EXD_SZSE) {
            if (ExecType == TORA_TSTP_ECT_Fill) {
                ModifyOrder(SecurityID, TradeVolume, BuyNo, TORA_TSTP_LSD_Buy);
                ModifyOrder(SecurityID, TradeVolume, SellNo, TORA_TSTP_LSD_Sell);
                PostPrice(SecurityID, TradePrice);
            } else if (ExecType == TORA_TSTP_ECT_Cancel) {
                if (BuyNo > 0) ModifyOrder(SecurityID, 0, BuyNo, TORA_TSTP_LSD_Buy);
                if (SellNo > 0) ModifyOrder(SecurityID, 0, SellNo, TORA_TSTP_LSD_Sell);
            }
        }
    }

    void MDL2Impl::NGTSTick(TTORATstpSecurityIDType SecurityID, TTORATstpLTickTypeType TickType, TTORATstpLongSequenceType BuyNo,
                            TTORATstpLongSequenceType SellNo, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume,
                            TTORATstpLSideType Side, TTORATstpTimeStampType TickTime) {
        if (TickType == TORA_TSTP_LTT_Add) {
            InsertOrder(SecurityID, Side==TORA_TSTP_LSD_Buy?BuyNo:SellNo, Price, Volume, Side);
        } else if (TickType == TORA_TSTP_LTT_Delete) {
            ModifyOrder(SecurityID, 0, Side==TORA_TSTP_LSD_Buy?BuyNo:SellNo, Side);
        } else if (TickType == TORA_TSTP_LTT_Trade) {
            ModifyOrder(SecurityID, Volume, BuyNo, TORA_TSTP_LSD_Buy);
            ModifyOrder(SecurityID, Volume, SellNo, TORA_TSTP_LSD_Sell);
            PostPrice(SecurityID, Price);
        }
    }

    void MDL2Impl::ClearOrder(TORALEV2API::TTORATstpLSideType Side) {
        MapOrder & mapOrder = Side==TORA_TSTP_LSD_Buy?m_orderBuy:m_orderSell;
        for (auto iter = mapOrder.begin(); iter != mapOrder.end(); ++iter) {
            for (auto iter1 = iter->second.begin(); iter1 != iter->second.end(); ++iter1) {
                for (auto iter2 = iter1->Orders.begin(); iter2 != iter1->Orders.end(); ++iter2) {
                    auto order = *iter2;
                    m_pool.Free<stOrder>(order, sizeof(stOrder));
                }
            }
        }
        mapOrder.clear();
    }

    void MDL2Impl::InsertOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType OrderNO, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpLSideType Side) {
        auto order = m_pool.Malloc<stOrder>(sizeof(stOrder));
        order->OrderNo = OrderNO;
        order->Volume = Volume;

        stPriceOrders priceOrder = {0};
        priceOrder.Orders.emplace_back(order);
        priceOrder.Price = Price;

        MapOrder &mapOrder = Side == TORA_TSTP_LSD_Buy ? m_orderBuy : m_orderSell;
        auto iter = mapOrder.find(SecurityID);
        if (iter == mapOrder.end()) {
            mapOrder[SecurityID] = std::vector<stPriceOrders>();
            mapOrder[SecurityID].emplace_back(priceOrder);
        } else if (iter->second.empty()) {
            iter->second.emplace_back(priceOrder);
        } else {
            auto size = (int) iter->second.size();
            for (auto i = 0; i < size; i++) {
                auto nextPrice = iter->second.at(i).Price;
                auto lastPrice = iter->second.at(size - 1).Price;
                if (Side == TORA_TSTP_LSD_Buy) {
                    if (Price > nextPrice + 0.000001) {
                        iter->second.insert(iter->second.begin() + i, priceOrder);
                        break;
                    } else if (Price > nextPrice - 0.000001) {
                        iter->second.at(i).Orders.emplace_back(order);
                        break;
                    }
                    if (i == size - 1 && Price < lastPrice - 0.000001) {
                        iter->second.emplace_back(priceOrder);
                    }
                } else if (Side == TORA_TSTP_LSD_Sell) {
                    if (Price < nextPrice - 0.000001) {
                        iter->second.insert(iter->second.begin() + i, priceOrder);
                        break;
                    } else if (Price < nextPrice + 0.000001) {
                        iter->second.at(i).Orders.emplace_back(order);
                        break;
                    }
                    if (i == size - 1 && Price > lastPrice + 0.000001) {
                        iter->second.emplace_back(priceOrder);
                    }
                }
            }
        }
    }

    bool MDL2Impl::ModifyOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongVolumeType Volume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType Side) {
        MapOrder &mapOrder = Side == TORA_TSTP_LSD_Buy ? m_orderBuy : m_orderSell;
        auto iter = mapOrder.find(SecurityID);
        if (iter != mapOrder.end()) {
            auto needReset = false, founded = false;
            for (auto i = 0; i < (int) iter->second.size(); i++) {
                for (auto j = 0; j < (int) iter->second.at(i).Orders.size(); j++) {
                    if (iter->second.at(i).Orders.at(j)->OrderNo == OrderNo) {
                        if (Volume == 0) {
                            iter->second.at(i).Orders.at(j)->Volume = 0;
                            needReset = true;
                        } else {
                            iter->second.at(i).Orders.at(j)->Volume -= Volume;
                            if (iter->second.at(i).Orders.at(j)->Volume <= 0) {
                                needReset = true;
                            }
                        }
                        founded = true;
                    }
                }
            }
            if (needReset) {
                ResetOrder(SecurityID, Side);
            }
            if (founded) {
                return true;
            }
        }
        return false;
    }

    void MDL2Impl::ResetOrder(TTORATstpSecurityIDType SecurityID, TTORATstpTradeBSFlagType Side) {
        MapOrder &mapOrder = Side == TORA_TSTP_LSD_Buy ? m_orderBuy : m_orderSell;
        auto iter = mapOrder.find(SecurityID);
        if (iter == mapOrder.end()) return;

        for (auto iterPriceOrder = iter->second.begin(); iterPriceOrder != iter->second.end();) {
            for (auto iterOrder = iterPriceOrder->Orders.begin(); iterOrder != iterPriceOrder->Orders.end();) {
                if ((*iterOrder)->Volume <= 0) {
                    auto order = *iterOrder;
                    iterOrder = iterPriceOrder->Orders.erase(iterOrder);
                    m_pool.Free<stOrder>(order, sizeof(stOrder));
                } else {
                    ++iterOrder;
                }
            }
            if (iterPriceOrder->Orders.empty()) {
                iterPriceOrder = iter->second.erase(iterPriceOrder);
            } else {
                ++iterPriceOrder;
            }
        }
    }

    void MDL2Impl::FixOrder(TTORATstpSecurityIDType SecurityID, TTORATstpPriceType Price, TTORATstpTimeStampType Time) {
        if (Time < 93000000) return;
        {
            auto reset = false;
            auto iter = m_orderSell.find(SecurityID);
            if (iter != m_orderSell.end()) {
                for (auto i = 0; i < (int)iter->second.size(); i++) {
                    if (iter->second.at(i).Price < Price) {
                        for (auto j = 0; j < (int)iter->second.at(i).Orders.size(); j++) {
                            iter->second.at(i).Orders.at(j)->Volume = 0;
                            reset = true;
                        }
                    }
                }
            }
            if (reset) ResetOrder(SecurityID, TORA_TSTP_LSD_Sell);
        }

        {
            auto reset = false;
            auto iter = m_orderBuy.find(SecurityID);
            if (iter != m_orderBuy.end()) {
                for (auto i = 0; i < (int)iter->second.size(); i++) {
                    if (iter->second.at(i).Price > Price) {
                        for (auto j = 0; j < (int)iter->second.at(i).Orders.size(); j++) {
                            iter->second.at(i).Orders.at(j)->Volume = 0;
                            reset = true;
                        }
                    }
                }
            }
            if (reset) ResetOrder(SecurityID, TORA_TSTP_LSD_Buy);
        }
    }

    void MDL2Impl::PostPrice(TTORATstpSecurityIDType SecurityID, TTORATstpPriceType Price) {
        if (!m_pApp || !m_pApp->m_strategyOpen) return;

        TTORATstpPriceType BidPrice1 = 0.0;
        TTORATstpLongVolumeType BidVolume1 = 0;
        TTORATstpPriceType AskPrice1 = 0.0;
        TTORATstpLongVolumeType AskVolume1 = 0;
        TTORATstpPriceType TradePrice = Price;

        {
            auto iter = m_orderSell.find(SecurityID);
            if (iter != m_orderSell.end() && !iter->second.empty()) {
                auto size = (int) iter->second.begin()->Orders.size();
                TTORATstpLongVolumeType Volume = 0;
                for (auto i = 0; i < size; ++i) {
                    Volume += iter->second.begin()->Orders.at(i)->Volume;
                }
                AskPrice1 = iter->second.begin()->Price;
                AskVolume1 = Volume;
            }
        }

        {
            auto iter = m_orderBuy.find(SecurityID);
            if (iter != m_orderBuy.end() && !iter->second.empty()) {
                auto size = (int) iter->second.begin()->Orders.size();
                TTORATstpLongVolumeType Volume = 0;
                for (auto i = 0; i < size; ++i) {
                    Volume += iter->second.begin()->Orders.at(i)->Volume;
                }
                BidPrice1 = iter->second.begin()->Price;
                BidVolume1 = Volume;
            }
        }

        stPostPrice p = {0};
        strcpy(p.SecurityID, SecurityID);
        p.BidVolume1 = BidVolume1;
        p.BidPrice1 = BidPrice1;
        p.AskPrice1 = AskPrice1;
        p.AskVolume1 = AskVolume1;
        p.TradePrice = TradePrice;
        m_pApp->m_ioc.post(boost::bind(&CApplication::MDPostPrice, m_pApp, p));
    }

}