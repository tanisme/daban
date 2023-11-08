#include "MDL2Impl.h"
#include "CApplication.h"

namespace PROMD {

    MDL2Impl::MDL2Impl(CApplication *pApp, TTORATstpExchangeIDType ExchangeID)
            : m_pApp(pApp), m_exchangeID(ExchangeID) {
    }

    MDL2Impl::~MDL2Impl() {
        if (m_pApi != nullptr) {
            m_pApi->Join();
            m_pApi->Release();
        }
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

    void MDL2Impl::ShowUnfindOrder(TTORATstpSecurityIDType SecurityID) {
        char buffer[512] = {0};
        {
            auto iter = m_unFindOrders.find(SecurityID);
            if (iter != m_unFindOrders.end()) {
                sprintf(buffer, "%s %s UnfindOrderCount:%d", GetTimeStr().c_str(), SecurityID, (int)iter->second.size());
                printf("%s\n", buffer);
            }
        }
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

    /************************************HandleOrderBook***************************************/
    void MDL2Impl::OnRtnOrderDetail(CTORATstpLev2OrderDetailField *pOrderDetail) {
        if (!pOrderDetail) return;
        if (pOrderDetail->Side != TORA_TSTP_LSD_Buy && pOrderDetail->Side != TORA_TSTP_LSD_Sell) return;

        if (pOrderDetail->ExchangeID == TORA_TSTP_EXD_SSE) {
            if (pOrderDetail->OrderStatus == TORA_TSTP_LOS_Add) {
                InsertOrder(pOrderDetail->SecurityID, pOrderDetail->OrderNO, pOrderDetail->Price, pOrderDetail->Volume, pOrderDetail->Side);
                HandleUnFindOrder(pOrderDetail->SecurityID, pOrderDetail->OrderNO, pOrderDetail->Side);
            } else if (pOrderDetail->OrderStatus == TORA_TSTP_LOS_Delete) {
                if (!ModifyOrder(pOrderDetail->SecurityID, 0, pOrderDetail->OrderNO, pOrderDetail->Side)) {
                    AddUnFindOrder(pOrderDetail->SecurityID, 0, pOrderDetail->OrderNO, pOrderDetail->Side, 1) ;
                }
            }
        } else if (pOrderDetail->ExchangeID == TORA_TSTP_EXD_SZSE) {
            InsertOrder(pOrderDetail->SecurityID, pOrderDetail->OrderNO, pOrderDetail->Price, pOrderDetail->Volume, pOrderDetail->Side);
            HandleUnFindOrder(pOrderDetail->SecurityID, pOrderDetail->OrderNO, pOrderDetail->Side);
        }
        //PostPrice(pOrderDetail->SecurityID, 0);
    }

    void MDL2Impl::OnRtnTransaction(CTORATstpLev2TransactionField *pTransaction) {
        if (!pTransaction) return;

        if (pTransaction->ExchangeID == TORA_TSTP_EXD_SSE) {
            if (!ModifyOrder(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->BuyNo, TORA_TSTP_LSD_Buy)) {
                AddUnFindOrder(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->BuyNo, TORA_TSTP_LSD_Buy);
            }
            if (!ModifyOrder(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->SellNo, TORA_TSTP_LSD_Sell)) {
                AddUnFindOrder(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->SellNo, TORA_TSTP_LSD_Sell);
            }
            //FixOrder(pTransaction->SecurityID, pTransaction->TradePrice, pTransaction->TradeTime);
        } else if (pTransaction->ExchangeID == TORA_TSTP_EXD_SZSE) {
            if (pTransaction->ExecType == TORA_TSTP_ECT_Fill) {
                if (!ModifyOrder(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->BuyNo, TORA_TSTP_LSD_Buy)) {
                    //AddUnFindOrder(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->BuyNo, TORA_TSTP_LSD_Buy);
                }
                if (!ModifyOrder(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->SellNo, TORA_TSTP_LSD_Sell)) {
                    //AddUnFindOrder(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->SellNo, TORA_TSTP_LSD_Sell);
                }
                //FixOrder(pTransaction->SecurityID, pTransaction->TradePrice, pTransaction->TradeTime);
            } else if (pTransaction->ExecType == TORA_TSTP_ECT_Cancel) {
                if (pTransaction->BuyNo > 0 && !ModifyOrder(pTransaction->SecurityID, 0, pTransaction->BuyNo, TORA_TSTP_LSD_Buy)) {
                    //AddUnFindOrder(pTransaction->SecurityID, 0, pTransaction->BuyNo, TORA_TSTP_LSD_Buy, 1);
                }
                if (pTransaction->SellNo > 0 && !ModifyOrder(pTransaction->SecurityID, 0, pTransaction->SellNo, TORA_TSTP_LSD_Sell)) {
                    //AddUnFindOrder(pTransaction->SecurityID, 0, pTransaction->SellNo, TORA_TSTP_LSD_Sell, 1);
                }
            }
        }
        PostPrice(pTransaction->SecurityID, pTransaction->TradePrice);
    }

    void MDL2Impl::OnRtnNGTSTick(CTORATstpLev2NGTSTickField *pTick) {
        if (!pTick) return;

        if (pTick->TickType == TORA_TSTP_LTT_Add) {
            InsertOrder(pTick->SecurityID, pTick->Side == TORA_TSTP_LSD_Buy?pTick->BuyNo:pTick->SellNo, pTick->Price, pTick->Volume, pTick->Side);
            HandleUnFindOrder(pTick->SecurityID, pTick->Side == TORA_TSTP_LSD_Buy?pTick->BuyNo:pTick->SellNo, pTick->Side);
        } else if (pTick->TickType == TORA_TSTP_LTT_Delete) {
            if (pTick->BuyNo > 0 && !ModifyOrder(pTick->SecurityID, 0, pTick->BuyNo, pTick->Side)) {
                AddUnFindOrder(pTick->SecurityID, 0, pTick->BuyNo, pTick->Side, 1);
            }
            if (pTick->SellNo > 0 && !ModifyOrder(pTick->SecurityID, 0, pTick->SellNo, pTick->Side)) {
                AddUnFindOrder(pTick->SecurityID, 0, pTick->SellNo, pTick->Side, 1);
            }
        } else if (pTick->TickType == TORA_TSTP_LTT_Trade) {
            if (!ModifyOrder(pTick->SecurityID, pTick->Volume, pTick->BuyNo, TORA_TSTP_LSD_Buy)) {
                AddUnFindOrder(pTick->SecurityID, pTick->Volume, pTick->BuyNo, TORA_TSTP_LSD_Buy);
            }
            if (!ModifyOrder(pTick->SecurityID, pTick->Volume, pTick->SellNo, TORA_TSTP_LSD_Sell)) {
                AddUnFindOrder(pTick->SecurityID, pTick->Volume, pTick->SellNo, TORA_TSTP_LSD_Sell);
            }
            //FixOrder(pTick->SecurityID, pTick->Price, pTick->TickTime);
        }
        PostPrice(pTick->SecurityID, pTick->Price);
    }

    void MDL2Impl::InsertOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType OrderNO, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpLSideType Side) {
        stOrder* order = m_pApp->m_pool.Malloc<stOrder>(sizeof(stOrder));
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
                    stOrder* order = *iterOrder;
                    iterOrder = iterPriceOrder->Orders.erase(iterOrder);
                    m_pApp->m_pool.Free<stOrder>(order, sizeof(stOrder));
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

    void MDL2Impl::AddUnFindOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongVolumeType Volume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType Side, int Type) {
        auto order = m_pApp->m_pool.Malloc<stUnfindOrder>(sizeof(stUnfindOrder));
        strcpy(order->SecurityID, SecurityID);
        order->OrderNo = OrderNo;
        order->Volume = Volume;
        order->Side = Side;
        order->Time = GetNowTick();
        order->Type = Type; // 0-trade 1-delete

        auto iter = m_unFindOrders.find(SecurityID);
        if (iter == m_unFindOrders.end()) {
            m_unFindOrders[SecurityID] = std::unordered_map<TTORATstpLongSequenceType, std::vector<stUnfindOrder*> >();
        }
        m_unFindOrders[SecurityID][OrderNo].emplace_back(order);
    }

    void MDL2Impl::HandleUnFindOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType Side) {
        auto nowTick = GetNowTick();
        {
            auto iter = m_unFindOrders.find(SecurityID);
            if (iter == m_unFindOrders.end()) return;

            for (auto iter1 = iter->second.begin(); iter1 != iter->second.end();) {
                for (auto iter2 = iter1->second.begin(); iter2 != iter1->second.end();) {
                    if ((*iter2)->Time + 30 <= nowTick) {
                        auto order = *iter2;
                        iter2 = iter1->second.erase(iter2);
                        m_pApp->m_pool.Free<stUnfindOrder>(order, sizeof(stUnfindOrder));
                        continue;
                    } else if ((*iter2)->OrderNo == OrderNo){
                        if ((*iter2)->Side == Side) {
                            if (ModifyOrder(SecurityID, (*iter2)->Volume, OrderNo, Side)) {
                                auto order = *iter2;
                                iter2 = iter1->second.erase(iter2);
                                m_pApp->m_pool.Free<stUnfindOrder>(order, sizeof(stUnfindOrder));
                                continue;
                            }
                        }
                    }
                    ++iter2;
                }
                if (iter1->second.empty()) {
                    iter1 = iter->second.erase(iter1);
                    continue;
                }
                ++iter1;
            }
            if (iter->second.empty()) {
                m_unFindOrders.erase(iter);
            }
        }
    }

    void MDL2Impl::PostPrice(TTORATstpSecurityIDType SecurityID, TTORATstpPriceType Price) {
        if (!m_pApp->m_strategyOpen) return;

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