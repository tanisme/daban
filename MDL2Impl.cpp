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

    bool MDL2Impl::Start(bool isTest, int version, int useQueueVersion) {
        m_version = version;
        m_useQueueVersion = useQueueVersion;

        if (m_useQueueVersion > 0) {
            m_stop = false;
            if (!m_pthread) {
                m_pthread = new std::thread(&MDL2Impl::Run, this);
            }
        }
        if (isTest) { // tcp
            m_pApi = CTORATstpLev2MdApi::CreateTstpLev2MdApi();
            m_pApi->RegisterSpi(this);
            if (m_exchangeID == TORA_TSTP_EXD_SSE) {
                m_pApi->RegisterFront((char *)m_pApp->m_testSHMDAddr.c_str());
                m_pApi->Init(m_pApp->m_shCpucore.c_str());
            } else if (m_exchangeID == TORA_TSTP_EXD_SZSE) {
                m_pApi->RegisterFront((char *)m_pApp->m_testSZMDAddr.c_str());
                m_pApi->Init(m_pApp->m_szCpucore.c_str());
            } else {
                return false;
            }
        } else { // udp
            m_pApi = CTORATstpLev2MdApi::CreateTstpLev2MdApi(TORA_TSTP_MST_MCAST);
            m_pApi->RegisterSpi(this);
            if (m_exchangeID == TORA_TSTP_EXD_SSE) {
                m_pApi->RegisterMulticast((char *)m_pApp->m_shMDAddr.c_str(), (char*)m_pApp->m_shMDInterface.c_str(), nullptr);
                m_pApi->Init(m_pApp->m_shCpucore.c_str());
            } else if (m_exchangeID == TORA_TSTP_EXD_SZSE) {
                m_pApi->RegisterMulticast((char *)m_pApp->m_szMDAddr.c_str(), (char*)m_pApp->m_szMDInterface.c_str(), nullptr);
                m_pApi->Init(m_pApp->m_szCpucore.c_str());
            } else {
                return false;
            }
        }
        printf("MD::Bind %s %s cpucore[%s]\n", isTest?"tcp":"udp", GetExchangeName(m_exchangeID),
               m_exchangeID == TORA_TSTP_EXD_SSE?m_pApp->m_shCpucore.c_str():m_pApp->m_szCpucore.c_str());
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

    void MDL2Impl::ShowHandleSpeed() {
        long long int cnt = 0;
        if (m_useQueueVersion == 0) cnt = m_addQueueCount;
        if (m_useQueueVersion == 1 || m_useQueueVersion == 2) cnt = m_delQueueCount;
        if (cnt <= 0) cnt = 1;
        printf("%s %s add:%-9lld handle:%-9lld us:%-9lld perus:%.6f %-9ld ver:%d qver:%d\n", GetTimeStr().c_str(), GetExchangeName(m_exchangeID),
               m_addQueueCount, m_delQueueCount, m_handleTick, (1.0*m_handleTick) / cnt, m_pool.GetTotalCnt(), m_version, m_useQueueVersion);
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

    /************************************API***************************************/
    void MDL2Impl::OnFrontConnected() {
        printf("%s MD::OnFrontConnected!!!\n", GetExchangeName(m_exchangeID));
        CTORATstpReqUserLoginField Req = {0};
        m_pApi->ReqUserLogin(&Req, ++m_reqID);
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
        printf("%s MD::OnRspUserLogin Success!!!\n", GetExchangeName(m_exchangeID));
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
        if (m_useQueueVersion == 0) {
            auto start = GetUs();
            OrderDetail(pOrderDetail->SecurityID, pOrderDetail->Side, pOrderDetail->OrderNO, pOrderDetail->Price, pOrderDetail->Volume, pOrderDetail->ExchangeID, pOrderDetail->OrderStatus);
            auto duration = GetUs() - start;
            if (duration <= 0) duration = 1; // default 1us
            m_handleTick += duration;
        } else if (m_useQueueVersion > 0){
            auto data = m_pool.Malloc<stNotifyData>(sizeof(stNotifyData));
            data->type = 1;
            strcpy(data->SecurityID, pOrderDetail->SecurityID);
            data->Side = pOrderDetail->Side;
            data->OrderNO = pOrderDetail->OrderNO;
            data->Price = pOrderDetail->Price;
            data->Volume = pOrderDetail->Volume;
            data->ExchangeID = pOrderDetail->ExchangeID;
            data->OrderStatus = pOrderDetail->OrderStatus;
            if (m_useQueueVersion == 1) {
                m_data.push(data);
            } else if (m_useQueueVersion == 2) {
                std::unique_lock<std::mutex> lock(m_dataMtx);
                m_dataList.push_back(data);
            }
        }
        m_addQueueCount++;
    }

    void MDL2Impl::OnRtnTransaction(CTORATstpLev2TransactionField *pTransaction) {
        if (!pTransaction) return;
        if (m_useQueueVersion == 0) {
            auto start = GetUs();
            Transaction(pTransaction->SecurityID, pTransaction->ExchangeID, pTransaction->TradeVolume, pTransaction->ExecType, pTransaction->BuyNo, pTransaction->SellNo, pTransaction->TradePrice, pTransaction->TradeTime);
            auto duration = GetUs() - start;
            if (duration <= 0) duration = 1;
            m_handleTick += duration;
        } else if (m_useQueueVersion > 0){
            auto data = m_pool.Malloc<stNotifyData>(sizeof(stNotifyData));
            data->type = 2;
            strcpy(data->SecurityID, pTransaction->SecurityID);
            data->ExecType = pTransaction->ExecType;
            data->ExchangeID = pTransaction->ExchangeID;
            data->Price = pTransaction->TradePrice;
            data->Volume = pTransaction->TradeVolume;
            data->BuyNo = pTransaction->BuyNo;
            data->SellNo = pTransaction->SellNo;
            if (m_useQueueVersion == 1) {
                m_data.push(data);
            } else if (m_useQueueVersion == 2) {
                std::unique_lock<std::mutex> lock(m_dataMtx);
                m_dataList.push_back(data);
            }
        }
        m_addQueueCount++;
    }

    void MDL2Impl::OnRtnNGTSTick(CTORATstpLev2NGTSTickField *pTick) {
        if (!pTick) return;
        if (m_useQueueVersion == 0) {
            auto start = GetUs();
            NGTSTick(pTick->SecurityID, pTick->TickType, pTick->BuyNo, pTick->SellNo, pTick->Price, pTick->Volume, pTick->Side, pTick->TickTime);
            auto duration = GetUs() - start;
            if (duration <= 0) duration = 1;
            m_handleTick += duration;
        } else if (m_useQueueVersion > 0){
            auto data = m_pool.Malloc<stNotifyData>(sizeof(stNotifyData));
            data->type = 3;
            strcpy(data->SecurityID, pTick->SecurityID);
            data->Side = pTick->Side;
            data->TickType = pTick->TickType;
            data->Price = pTick->Price;
            data->Volume = pTick->Volume;
            data->BuyNo = pTick->BuyNo;
            data->SellNo = pTick->SellNo;
            if (m_useQueueVersion == 1) {
                m_data.push(data);
            } else if (m_useQueueVersion == 2) {
                std::unique_lock<std::mutex> lock(m_dataMtx);
                m_dataList.push_back(data);
            }
        }
        m_addQueueCount++;
    }

    /************************************HandleOrderBook***************************************/
    void MDL2Impl::OrderDetail(TTORATstpSecurityIDType SecurityID, TTORATstpTradeBSFlagType Side, TTORATstpLongSequenceType OrderNO,
                               TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpExchangeIDType ExchangeID,
                               TTORATstpLOrderStatusType OrderStatus) {
        if (Side != TORA_TSTP_LSD_Buy && Side != TORA_TSTP_LSD_Sell) return;
        if (ExchangeID == TORA_TSTP_EXD_SSE) {
            if (OrderStatus == TORA_TSTP_LOS_Add) {
                if (m_version == 0) {
                    InsertOrderV(SecurityID, OrderNO, Price, Volume, Side);
                } else if (m_version == 1) {
                    InsertOrderL(SecurityID, OrderNO, Price, Volume, Side);
                } else if (m_version == 2) {
                    InsertOrderM(SecurityID, OrderNO, Price, Volume, Side);
                }
            } else if (OrderStatus == TORA_TSTP_LOS_Delete) {
                if (m_version == 0) {
                    ModifyOrderV(SecurityID, 0, OrderNO, Side);
                } else if (m_version == 1) {
                    ModifyOrderL(SecurityID, 0, OrderNO, Side);
                } else if (m_version == 2) {
                    ModifyOrderM(SecurityID, 0, OrderNO, Side);
                }
            }
        } else if (ExchangeID == TORA_TSTP_EXD_SZSE) {
            if (m_version == 0) {
                InsertOrderV(SecurityID, OrderNO, Price, Volume, Side);
            } else if (m_version == 1) {
                InsertOrderL(SecurityID, OrderNO, Price, Volume, Side);
            } else if (m_version == 2) {
                InsertOrderM(SecurityID, OrderNO, Price, Volume, Side);
            }
        }
    }

    void MDL2Impl::Transaction(TTORATstpSecurityIDType SecurityID, TTORATstpExchangeIDType ExchangeID, TTORATstpLongVolumeType TradeVolume,
                               TTORATstpExecTypeType ExecType, TTORATstpLongSequenceType BuyNo, TTORATstpLongSequenceType SellNo,
                               TTORATstpPriceType TradePrice, TTORATstpTimeStampType TradeTime) {
        if (ExchangeID == TORA_TSTP_EXD_SSE) {
            if (m_version == 0) {
                ModifyOrderV(SecurityID, TradeVolume, BuyNo, TORA_TSTP_LSD_Buy);
                ModifyOrderV(SecurityID, TradeVolume, SellNo, TORA_TSTP_LSD_Sell);
                PostPriceV(SecurityID, TradePrice);
            } else if (m_version == 1) {
                ModifyOrderL(SecurityID, TradeVolume, BuyNo, TORA_TSTP_LSD_Buy);
                ModifyOrderL(SecurityID, TradeVolume, SellNo, TORA_TSTP_LSD_Sell);
                PostPriceL(SecurityID, TradePrice);
            } else if (m_version == 2) {
                ModifyOrderM(SecurityID, TradeVolume, BuyNo, TORA_TSTP_LSD_Buy);
                ModifyOrderM(SecurityID, TradeVolume, SellNo, TORA_TSTP_LSD_Sell);
                PostPriceM(SecurityID, TradePrice);
            }
        } else if (ExchangeID == TORA_TSTP_EXD_SZSE) {
            if (ExecType == TORA_TSTP_ECT_Fill) {
                if (m_version == 0) {
                    ModifyOrderV(SecurityID, TradeVolume, BuyNo, TORA_TSTP_LSD_Buy);
                    ModifyOrderV(SecurityID, TradeVolume, SellNo, TORA_TSTP_LSD_Sell);
                    PostPriceV(SecurityID, TradePrice);
                } else if (m_version == 1) {
                    ModifyOrderL(SecurityID, TradeVolume, BuyNo, TORA_TSTP_LSD_Buy);
                    ModifyOrderL(SecurityID, TradeVolume, SellNo, TORA_TSTP_LSD_Sell);
                    PostPriceL(SecurityID, TradePrice);
                } else if (m_version == 2) {
                    ModifyOrderM(SecurityID, TradeVolume, BuyNo, TORA_TSTP_LSD_Buy);
                    ModifyOrderM(SecurityID, TradeVolume, SellNo, TORA_TSTP_LSD_Sell);
                    PostPriceM(SecurityID, TradePrice);
                }
            } else if (ExecType == TORA_TSTP_ECT_Cancel) {
                if (BuyNo > 0) {
                    if (m_version == 0) {
                        ModifyOrderV(SecurityID, 0, BuyNo, TORA_TSTP_LSD_Buy);
                    } else if (m_version == 1) {
                        ModifyOrderL(SecurityID, 0, BuyNo, TORA_TSTP_LSD_Buy);
                    } else if (m_version == 2) {
                        ModifyOrderM(SecurityID, 0, BuyNo, TORA_TSTP_LSD_Buy);
                    }
                }
                if (SellNo > 0) {
                    if (m_version == 0) {
                        ModifyOrderV(SecurityID, 0, SellNo, TORA_TSTP_LSD_Sell);
                    } else if (m_version == 1) {
                        ModifyOrderL(SecurityID, 0, SellNo, TORA_TSTP_LSD_Sell);
                    } else if (m_version == 2) {
                        ModifyOrderM(SecurityID, 0, SellNo, TORA_TSTP_LSD_Sell);
                    }
                }
            }
        }
    }

    void MDL2Impl::NGTSTick(TTORATstpSecurityIDType SecurityID, TTORATstpLTickTypeType TickType, TTORATstpLongSequenceType BuyNo,
                            TTORATstpLongSequenceType SellNo, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume,
                            TTORATstpLSideType Side, TTORATstpTimeStampType TickTime) {
        if (TickType == TORA_TSTP_LTT_Add) {
            if (m_version == 0) {
                InsertOrderV(SecurityID, Side==TORA_TSTP_LSD_Buy?BuyNo:SellNo, Price, Volume, Side);
            } else if (m_version == 1) {
                InsertOrderL(SecurityID, Side==TORA_TSTP_LSD_Buy?BuyNo:SellNo, Price, Volume, Side);
            } else if (m_version == 2) {
                InsertOrderM(SecurityID, Side==TORA_TSTP_LSD_Buy?BuyNo:SellNo, Price, Volume, Side);
            }
        } else if (TickType == TORA_TSTP_LTT_Delete) {
            if (m_version == 0) {
                ModifyOrderV(SecurityID, 0, Side==TORA_TSTP_LSD_Buy?BuyNo:SellNo, Side);
            } else if (m_version == 1) {
                ModifyOrderL(SecurityID, 0, Side==TORA_TSTP_LSD_Buy?BuyNo:SellNo, Side);
            } else if (m_version == 2) {
                ModifyOrderM(SecurityID, 0, Side==TORA_TSTP_LSD_Buy?BuyNo:SellNo, Side);
            }
        } else if (TickType == TORA_TSTP_LTT_Trade) {
            if (m_version == 0) {
                ModifyOrderV(SecurityID, Volume, BuyNo, TORA_TSTP_LSD_Buy);
                ModifyOrderV(SecurityID, Volume, SellNo, TORA_TSTP_LSD_Sell);
                PostPriceV(SecurityID, Price);
            } else if (m_version == 1) {
                ModifyOrderL(SecurityID, Volume, BuyNo, TORA_TSTP_LSD_Buy);
                ModifyOrderL(SecurityID, Volume, SellNo, TORA_TSTP_LSD_Sell);
                PostPriceL(SecurityID, Price);
            } else if (m_version == 2) {
                ModifyOrderM(SecurityID, Volume, BuyNo, TORA_TSTP_LSD_Buy);
                ModifyOrderM(SecurityID, Volume, SellNo, TORA_TSTP_LSD_Sell);
                PostPriceM(SecurityID, Price);
            }
        }
    }

    /******************************Vector******************************/
    void MDL2Impl::InsertOrderV(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType OrderNO, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpLSideType Side) {
        auto order = m_pool.Malloc<stOrder>(sizeof(stOrder));
        order->OrderNo = OrderNO;
        order->Volume = Volume;

        stPriceOrders priceOrder = {0};
        priceOrder.Orders.emplace_back(order);
        priceOrder.Price = Price;

        auto &mapOrder = Side == TORA_TSTP_LSD_Buy ? m_orderBuyV : m_orderSellV;
        auto iter = mapOrder.find(SecurityID);
        if (iter == mapOrder.end()) {
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

    void MDL2Impl::ModifyOrderV(TTORATstpSecurityIDType SecurityID, TTORATstpLongVolumeType Volume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType Side) {
        auto &mapOrder = Side == TORA_TSTP_LSD_Buy ? m_orderBuyV : m_orderSellV;
        auto iter = mapOrder.find(SecurityID);
        if (iter == mapOrder.end()) return;

        for (auto iterVecOrder = iter->second.begin(); iterVecOrder != iter->second.end();) {
            auto nb = false;
            for (auto iterOrder = iterVecOrder->Orders.begin(); iterOrder != iterVecOrder->Orders.end();) {
                if ((*iterOrder)->OrderNo == OrderNo) {
                    if (Volume > 0) {
                        (*iterOrder)->Volume -= Volume;
                    } else {
                        nb = true;
                    }
                    if (Volume == 0 || (*iterOrder)->Volume <= 0) {
                        auto order = (*iterOrder);
                        iterOrder = iterVecOrder->Orders.erase(iterOrder);
                        m_pool.Free<stOrder>(order, sizeof(stOrder));
                    } else {
                        ++iterOrder;
                    }
                } else {
                    ++iterOrder;
                }
                if (nb) break;
            }
            if (iterVecOrder->Orders.empty()) {
                iterVecOrder = iter->second.erase(iterVecOrder);
            } else {
                ++iterVecOrder;
            }
            if (nb) break;
        }
    }

    void MDL2Impl::ShowOrderBookV(TTORATstpSecurityIDType SecurityID) {
        if (m_orderBuyV.find(SecurityID) == m_orderBuyV.end() &&
            m_orderSellV.find(SecurityID) == m_orderSellV.end()) return;

        std::stringstream stream;
        stream << "--------- " << SecurityID << " " << GetTimeStr() << "---------\n";

        char buffer[512] = {0};
        int showPankouCount = 5;
        {
            auto iter = m_orderSellV.find(SecurityID);
            if (iter != m_orderSellV.end()) {
                auto size = (int) iter->second.size();
                if (showPankouCount < size) size = showPankouCount;
                for (auto i = size; i > 0; i--) {
                    memset(buffer, 0, sizeof(buffer));
                    TTORATstpLongVolumeType totalVolume = 0;
                    for (auto iter1: iter->second.at(i - 1).Orders) totalVolume += iter1->Volume;
                    sprintf(buffer, "S%d\t%.3f\t\t%lld\t%d\n", i, iter->second.at(i - 1).Price,
                            totalVolume, (int) iter->second.at(i - 1).Orders.size());
                    stream << buffer;
                }
            }
        }

        {
            auto iter = m_orderBuyV.find(SecurityID);
            if (iter != m_orderBuyV.end()) {
                auto size = (int) iter->second.size();
                if (showPankouCount < size) size = showPankouCount;
                for (auto i = 1; i <= size; i++) {
                    memset(buffer, 0, sizeof(buffer));
                    TTORATstpLongVolumeType totalVolume = 0;
                    for (auto iter1: iter->second.at(i - 1).Orders) totalVolume += iter1->Volume;
                    sprintf(buffer, "B%d\t%.3f\t\t%lld\t%d\n", i, iter->second.at(i - 1).Price,
                            totalVolume, (int) iter->second.at(i - 1).Orders.size());
                    stream << buffer;
                }
            }
        }
        printf("%s\n", stream.str().c_str());
    }

    void MDL2Impl::PostPriceV(TTORATstpSecurityIDType SecurityID, TTORATstpPriceType Price) {
        if (!m_pApp || !m_pApp->m_isStrategyOpen) return;

        TTORATstpPriceType BidPrice1 = 0.0;
        TTORATstpLongVolumeType BidVolume1 = 0;
        TTORATstpPriceType AskPrice1 = 0.0;
        TTORATstpLongVolumeType AskVolume1 = 0;
        TTORATstpPriceType TradePrice = Price;

        {
            auto iter = m_orderSellV.find(SecurityID);
            if (iter != m_orderSellV.end() && !iter->second.empty()) {
                auto size = (int) iter->second.begin()->Orders.size();
                TTORATstpLongVolumeType Volume = 0;
                for (auto i = 0; i < size; ++i) Volume += iter->second.begin()->Orders.at(i)->Volume;
                AskPrice1 = iter->second.begin()->Price;
                AskVolume1 = Volume;
            }
        }

        {
            auto iter = m_orderBuyV.find(SecurityID);
            if (iter != m_orderBuyV.end() && !iter->second.empty()) {
                auto size = (int) iter->second.begin()->Orders.size();
                TTORATstpLongVolumeType Volume = 0;
                for (auto i = 0; i < size; ++i) Volume += iter->second.begin()->Orders.at(i)->Volume;
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

    /******************************List******************************/
    void MDL2Impl::InsertOrderL(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType OrderNO, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpLSideType Side) {
        auto order = m_pool.Malloc<stOrder>(sizeof(stOrder));
        order->OrderNo = OrderNO;
        order->Volume = Volume;

        int PriceInt = (int)(Price * 100);

        auto &mapOrder = Side == TORA_TSTP_LSD_Buy ? m_orderBuyL : m_orderSellL;
        auto iter = mapOrder.find(SecurityID);
        if (iter == mapOrder.end()) {
            mapOrder[SecurityID] = std::map<int, std::vector<stOrder*> >();
        }
        mapOrder[SecurityID][PriceInt].emplace_back(order);
    }

    void MDL2Impl::ModifyOrderL(TTORATstpSecurityIDType SecurityID, TTORATstpLongVolumeType Volume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType Side) {
        auto &mapOrder = Side == TORA_TSTP_LSD_Buy ? m_orderBuyL : m_orderSellL;
        auto iter = mapOrder.find(SecurityID);
        if (iter == mapOrder.end()) return;

        for (auto iterOrderList = iter->second.begin(); iterOrderList != iter->second.end();) {
            for (auto order = iterOrderList->second.begin(); order != iterOrderList->second.end();) {
                if ((*order)->OrderNo == OrderNo) {
                    if (Volume > 0) (*order)->Volume -= Volume;
                    if (Volume == 0 || (*order)->Volume <= 0) {
                        auto tmp = (*order);
                        order = iterOrderList->second.erase(order);
                        m_pool.Free<stOrder>(tmp, sizeof(stOrder));
                    } else {
                        ++order;
                    }
                } else {
                    ++order;
                }
            }

            if (iterOrderList->second.empty()) {
                iterOrderList = iter->second.erase(iterOrderList);
            } else {
                ++iterOrderList;
            }
        }
    }

    void MDL2Impl::ShowOrderBookL(TTORATstpSecurityIDType SecurityID) {
        if (m_orderBuyL.find(SecurityID) == m_orderBuyL.end() &&
            m_orderSellL.find(SecurityID) == m_orderSellL.end()) return;

        std::stringstream stream;
        stream << "--------- " << SecurityID << " " << GetTimeStr() << "---------\n";

        char buffer[512] = {0};
        {
            std::vector<std::string> temp;
            auto iter = m_orderSellL.find(SecurityID);
            if (iter != m_orderSellL.end()) {
                int i = 1;
                for (auto iter1: iter->second) {
                    memset(buffer, 0, sizeof(buffer));
                    TTORATstpLongVolumeType totalVolume = 0;
                    for (auto iter2 : iter1.second) {
                        totalVolume += iter2->Volume;
                    }
                    auto left = totalVolume % 100;
                    auto cnt = totalVolume / 100;
                    if (left >= 50) cnt++;
                    totalVolume = cnt;
                    sprintf(buffer, "S%d\t%.2f\t\t%lld\t%d\n", i, iter1.first/100.0, totalVolume, (int)iter1.second.size());
                    temp.emplace_back(buffer);
                    if (i++ >= 5) break;
                }
                for (auto it = temp.rbegin(); it != temp.rend(); ++it) {
                    stream << (*it);
                }
            }
        }

        {
            std::vector<std::string> temp;
            auto iter = m_orderBuyL.find(SecurityID);
            if (iter != m_orderBuyL.end()) {
                int i = 1;
                for (auto iter1: iter->second) {
                    memset(buffer, 0, sizeof(buffer));
                    TTORATstpLongVolumeType totalVolume = 0;
                    for (auto iter2 : iter1.second) {
                        totalVolume += iter2->Volume;
                    }
                    auto left = totalVolume % 100;
                    auto cnt = totalVolume / 100;
                    if (left >= 50) cnt++;
                    totalVolume = cnt;
                    sprintf(buffer, "B%d\t%.2f\t\t%lld\t%d\n", i, iter1.first/100.0, totalVolume, (int)iter1.second.size());
                    temp.emplace_back(buffer);
                    if (i++ >= 5) break;
                }
                for (auto it = temp.begin(); it != temp.end(); ++it) {
                    stream << (*it);
                }
            }
        }
        printf("%s\n", stream.str().c_str());
    }

    void MDL2Impl::PostPriceL(TTORATstpSecurityIDType SecurityID, TTORATstpPriceType Price) {
        if (!m_pApp || !m_pApp->m_isStrategyOpen) return;

        TTORATstpPriceType BidPrice1 = 0.0;
        TTORATstpLongVolumeType BidVolume1 = 0;
        TTORATstpPriceType AskPrice1 = 0.0;
        TTORATstpLongVolumeType AskVolume1 = 0;
        TTORATstpPriceType TradePrice = Price;

        {
            auto iter = m_orderSellV.find(SecurityID);
            if (iter != m_orderSellV.end() && !iter->second.empty()) {
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
            auto iter = m_orderBuyV.find(SecurityID);
            if (iter != m_orderBuyV.end() && !iter->second.empty()) {
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

    /******************************Map******************************/
    void MDL2Impl::InsertOrderM(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType OrderNO, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpLSideType Side) {
        auto order = m_pool.Malloc<stOrder>(sizeof(stOrder));
        order->OrderNo = OrderNO;
        order->Volume = Volume;

        auto added = false;
        auto &mapOrder = Side == TORA_TSTP_LSD_Buy ? m_orderBuyM : m_orderSellM;
        auto iter = mapOrder.find(SecurityID);
        if (iter == mapOrder.end()) {
            mapOrder[SecurityID] = std::list<stPriceOrdersMap>();
            iter = mapOrder.find(SecurityID);
        } else if (!iter->second.empty()) {
            for (auto iterOrderList = iter->second.begin(); iterOrderList != iter->second.end(); ++iterOrderList) {
                auto curPrice = iterOrderList->Price;
                if (Side == TORA_TSTP_LSD_Buy) {
                    if (Price > curPrice) {
                        stPriceOrdersMap priceOrder = {0};
                        priceOrder.Orders[OrderNO] = std::list<stOrder*>();
                        priceOrder.Orders[OrderNO].push_back(order);
                        priceOrder.Price = Price;
                        iter->second.insert(iterOrderList, priceOrder);
                        added = true;
                        break;
                    } else if (Price == curPrice) {
                        if (iterOrderList->Orders.find(OrderNO) == iterOrderList->Orders.end()) {
                            iterOrderList->Orders[OrderNO] = std::list<stOrder*>();
                        }
                        iterOrderList->Orders[OrderNO].emplace_back(order);
                        added = true;
                        break;
                    }
                } else if (Side == TORA_TSTP_LSD_Sell) {
                    if (Price < curPrice) {
                        stPriceOrdersMap priceOrder = {0};
                        priceOrder.Orders[OrderNO] = std::list<stOrder*>();
                        priceOrder.Orders[OrderNO].push_back(order);
                        priceOrder.Price = Price;
                        iter->second.insert(iterOrderList, priceOrder);
                        added = true;
                        break;
                    } else if (Price == curPrice) {
                        if (iterOrderList->Orders.find(OrderNO) == iterOrderList->Orders.end()) {
                            iterOrderList->Orders[OrderNO] = std::list<stOrder*>();
                        }
                        iterOrderList->Orders[OrderNO].emplace_back(order);
                        added = true;
                        break;
                    }
                }
            }
        }
        if (!added) {
            stPriceOrdersMap priceOrder = {0};
            priceOrder.Orders[OrderNO] = std::list<stOrder*>();
            priceOrder.Orders[OrderNO].push_back(order);
            priceOrder.Price = Price;
            iter->second.emplace_back(priceOrder);
        }
    }

    void MDL2Impl::ModifyOrderM(TTORATstpSecurityIDType SecurityID, TTORATstpLongVolumeType Volume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType Side) {
        auto &mapOrder = Side == TORA_TSTP_LSD_Buy ? m_orderBuyM : m_orderSellM;
        auto iter = mapOrder.find(SecurityID);
        if (iter == mapOrder.end()) return;

        for (auto iterOrderList = iter->second.begin(); iterOrderList != iter->second.end();) {
            auto nb = false;
            if (iterOrderList->Orders.find(OrderNo) != iterOrderList->Orders.end()) {
                for (auto order = iterOrderList->Orders[OrderNo].begin(); order != iterOrderList->Orders[OrderNo].end();) {
                    if (Volume > 0) (*order)->Volume -= Volume;
                    if (Volume == 0 || (*order)->Volume <= 0) {
                        auto tmp = (*order);
                        order = iterOrderList->Orders[OrderNo].erase(order);
                        m_pool.Free<stOrder>(tmp, sizeof(stOrder));
                    } else {
                        ++order;
                    }
                }
                if (iterOrderList->Orders[OrderNo].empty()) {
                    iterOrderList->Orders.erase(OrderNo);
                }
                nb = true;
            }
            if (iterOrderList->Orders.empty()) {
                iterOrderList = iter->second.erase(iterOrderList);
            } else {
                ++iterOrderList;
            }
            if (nb) break;
        }
    }

    void MDL2Impl::ShowOrderBookM(TTORATstpSecurityIDType SecurityID) {
        if (m_orderBuyM.find(SecurityID) == m_orderBuyM.end() && m_orderSellM.find(SecurityID) == m_orderSellM.end()) return;

        std::stringstream stream;
        stream << "\n";
        stream << "--------- " << SecurityID << " " << GetTimeStr() << "---------\n";

        char buffer[512] = {0};
        int showPankouCount = 5;
        {
            std::vector<std::string> temp;
            auto iter = m_orderSellM.find(SecurityID);
            if (iter != m_orderSellM.end()) {
                int i = 1;
                if (i > showPankouCount) i = showPankouCount;
                for (auto iter1: iter->second) {
                    int cnt = 0;
                    memset(buffer, 0, sizeof(buffer));
                    TTORATstpLongVolumeType totalVolume = 0;
                    for (auto iter2 : iter1.Orders) {
                        for (auto iter3 : iter2.second) {
                            totalVolume += iter3->Volume;
                            cnt++;
                        }
                    }
                    sprintf(buffer, "S%d\t%.3f\t\t%lld\t%d\n", i, iter1.Price, totalVolume, cnt);
                    temp.emplace_back(buffer);
                    if (i++ >= 5) break;
                }
                for (auto it = temp.rbegin(); it != temp.rend(); ++it) {
                    stream << (*it);
                }
            }
        }

        {

            std::vector<std::string> temp;
            auto iter = m_orderBuyM.find(SecurityID);
            if (iter != m_orderBuyM.end()) {
                int i = 1;
                if (i > showPankouCount) i = showPankouCount;
                for (auto iter1: iter->second) {
                    int cnt = 0;
                    memset(buffer, 0, sizeof(buffer));
                    TTORATstpLongVolumeType totalVolume = 0;
                    for (auto iter2 : iter1.Orders) {
                        for (auto iter3 : iter2.second) {
                            totalVolume += iter3->Volume;
                            cnt++;
                        }
                    }
                    sprintf(buffer, "B%d\t%.3f\t\t%lld\t%d\n", i, iter1.Price, totalVolume, cnt);
                    temp.emplace_back(buffer);
                    if (i++ >= 5) break;
                }
                for (auto it = temp.begin(); it != temp.end(); ++it) {
                    stream << (*it);
                }
            }
        }
        printf("%s\n", stream.str().c_str());
    }

    void MDL2Impl::PostPriceM(TTORATstpSecurityIDType SecurityID, TTORATstpPriceType Price) {
        if (!m_pApp || !m_pApp->m_isStrategyOpen) return;

        TTORATstpPriceType BidPrice1 = 0.0;
        TTORATstpLongVolumeType BidVolume1 = 0;
        TTORATstpPriceType AskPrice1 = 0.0;
        TTORATstpLongVolumeType AskVolume1 = 0;
        TTORATstpPriceType TradePrice = Price;

        {
            auto iter = m_orderSellV.find(SecurityID);
            if (iter != m_orderSellV.end() && !iter->second.empty()) {
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
            auto iter = m_orderBuyV.find(SecurityID);
            if (iter != m_orderBuyV.end() && !iter->second.empty()) {
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

    void MDL2Impl::Run() {
        while (!m_stop) {
            if (m_useQueueVersion == 1) {
                stNotifyData* data = nullptr;
                m_data.pop(data);
                if (data) {
                    auto start = GetUs();
                    HandleData(data);
                    auto duration = GetUs() - start;
                    if (duration <= 0) duration = 1; // default 1us
                    m_delQueueCount++;
                    m_handleTick += duration;
                    m_pool.Free(data, sizeof(stNotifyData));
                }
            } else if (m_useQueueVersion == 2) {
                std::list<stNotifyData*> dataList;
                {
                    std::unique_lock<std::mutex> lock(m_dataMtx);
                    dataList.swap(m_dataList);
                    m_dataList.clear();
                    if (dataList.empty()) continue;
                }

                auto start = GetUs();
                for (auto iter = dataList.begin(); iter != dataList.end(); ++iter) {
                    auto data = *iter;
                    HandleData(data);
                    m_delQueueCount++;
                    m_pool.Free(data, sizeof(stNotifyData));
                }
                auto duration = GetUs() - start;
                if (duration <= 0) duration = 1; // default 1us
                m_handleTick += duration;
            }
        }
    }

    void MDL2Impl::HandleData(stNotifyData *data) {
        if (!data) return;
        if (data->type == 1) {
            OrderDetail(data->SecurityID, data->Side, data->OrderNO, data->Price, data->Volume, data->ExchangeID, data->OrderStatus);
        } else if (data->type == 2) {
            Transaction(data->SecurityID, data->ExchangeID, data->Volume, data->ExecType, data->BuyNo, data->SellNo, data->Price, 0);
        } else if (data->type == 3) {
            NGTSTick(data->SecurityID, data->TickType, data->BuyNo, data->SellNo, data->Price, data->Volume, data->Side, 0);
        }
    }

}