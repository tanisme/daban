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

    bool MDL2Impl::Start(bool isTest, bool useVec) {
        m_useVec = useVec;

        m_stop = false;
        if (!m_pthread) {
            m_pthread = new std::thread(&MDL2Impl::Run, this);
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
        if (m_delQueueCount <= 0) return;
        printf("%s %s %s %-9lld %-9lld %-9lld %.3fus %ld\n", m_useVec?"V":"M", GetExchangeName(m_exchangeID), GetTimeStr().c_str(),
               m_addQueueCount, m_delQueueCount, m_handleTick, (1.0*m_handleTick) / m_delQueueCount, m_pool.GetTotalCnt());
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
        auto data = m_pool.Malloc<stNotifyData>(sizeof(stNotifyData));
        data->type = 1;
        strcpy(data->SecurityID, pOrderDetail->SecurityID);
        data->Side = pOrderDetail->Side;
        data->OrderNO = pOrderDetail->OrderNO;
        data->Price = pOrderDetail->Price;
        data->Volume = pOrderDetail->Volume;
        data->ExchangeID = pOrderDetail->ExchangeID;
        data->OrderStatus = pOrderDetail->OrderStatus;
        m_addQueueCount++;
        //check loss order
        //if (m_seq.find(pOrderDetail->MainSeq) == m_seq.end()) {
        //    m_seq[pOrderDetail->MainSeq] = pOrderDetail->SubSeq;
        //} else {
        //    auto subseq = m_seq[pOrderDetail->MainSeq] + 1;
        //    if (subseq != pOrderDetail->SubSeq) {
        //        printf("loss order %d\n", subseq);
        //        m_seq[pOrderDetail->MainSeq] = pOrderDetail->SubSeq;
        //    }
        //}
        //printf("OnRtnOrderDetail %s MainSeq:%d SubSeq:%d\n",pOrderDetail->SecurityID, pOrderDetail->MainSeq, pOrderDetail->SubSeq);
        //m_data.push(data);
        std::unique_lock<std::mutex> lock(m_dataMtx);
        m_dataList.push_back(data);
        m_waitQueueCount = m_dataList.size();
    }

    void MDL2Impl::OnRtnTransaction(CTORATstpLev2TransactionField *pTransaction) {
        if (!pTransaction) return;
        auto data = m_pool.Malloc<stNotifyData>(sizeof(stNotifyData));
        data->type = 2;
        strcpy(data->SecurityID, pTransaction->SecurityID);
        data->ExecType = pTransaction->ExecType;
        data->ExchangeID = pTransaction->ExchangeID;
        data->Price = pTransaction->TradePrice;
        data->Volume = pTransaction->TradeVolume;
        data->BuyNo = pTransaction->BuyNo;
        data->SellNo = pTransaction->SellNo;
        m_addQueueCount++;
        //printf("OnRtnTransaction SubSeq:%lld %s\n", pTransaction->SubSeq, pTransaction->SecurityID);
        //m_data.push(data);
        std::unique_lock<std::mutex> lock(m_dataMtx);
        m_dataList.push_back(data);
        m_waitQueueCount = m_dataList.size();
    }

    void MDL2Impl::OnRtnNGTSTick(CTORATstpLev2NGTSTickField *pTick) {
        if (!pTick) return;
        auto data = m_pool.Malloc<stNotifyData>(sizeof(stNotifyData));
        data->type = 3;
        strcpy(data->SecurityID, pTick->SecurityID);
        data->Side = pTick->Side;
        data->TickType = pTick->TickType;
        data->Price = pTick->Price;
        data->Volume = pTick->Volume;
        data->BuyNo = pTick->BuyNo;
        data->SellNo = pTick->SellNo;
        data->ExchangeID = pTick->ExchangeID;
        m_addQueueCount++;
        //printf("OnRtnNGTSTick SubSeq:%lld %s\n", pTick->SubSeq, pTick->SecurityID);
        //m_data.push(data);
        std::unique_lock<std::mutex> lock(m_dataMtx);
        m_dataList.push_back(data);
        m_waitQueueCount = m_dataList.size();
    }

    void MDL2Impl::Run() {
        while (!m_stop) {
#if 0
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
#endif
            std::list<stNotifyData*> dataList;
            {
                std::unique_lock<std::mutex> lock(m_dataMtx);
                if (m_dataList.empty()) continue;
                dataList.swap(m_dataList);
            }

            auto size = dataList.size();
            if (size <= 0) {
                //std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            auto start = GetUs();
            for (auto iter = dataList.begin(); iter != dataList.end(); ++iter) {
                auto data = *iter;
                if (!data) continue;
                HandleData(data);
                m_pool.Free(data, sizeof(stNotifyData));
            }
            auto duration = GetUs() - start;
            if (duration <= 0) duration = 1; // default 1us
            m_delQueueCount += (long long int)size;
            m_handleTick += duration;
        }
    }

    void MDL2Impl::HandleData(stNotifyData *data) {
        if (!data) return;
        if (data->type == 1) {
            OrderDetail(data->SecurityID, data->Side, data->OrderNO, data->Price, data->Volume, data->ExchangeID, data->OrderStatus);
        } else if (data->type == 2) {
            Transaction(data->SecurityID, data->ExchangeID, data->Volume, data->ExecType, data->BuyNo, data->SellNo, data->Price, 0);
        } else if (data->type == 3) {
            NGTSTick(data->SecurityID, data->ExchangeID, data->TickType, data->BuyNo, data->SellNo, data->Price, data->Volume, data->Side, 0);
        }
    }

    /************************************HandleOrderBook***************************************/
    void MDL2Impl::OrderDetail(TTORATstpSecurityIDType SecurityID, TTORATstpTradeBSFlagType Side, TTORATstpLongSequenceType OrderNO,
                               TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpExchangeIDType ExchangeID,
                               TTORATstpLOrderStatusType OrderStatus) {
        if (Side != TORA_TSTP_LSD_Buy && Side != TORA_TSTP_LSD_Sell) return;
        if (ExchangeID == TORA_TSTP_EXD_SZSE) {
            if (m_useVec) {
                InsertOrderV(SecurityID, OrderNO, Price, Volume, Side);
            } else {
                InsertOrderM(SecurityID, OrderNO, Price, Volume, Side);
            }
        } 
		//else if (ExchangeID == TORA_TSTP_EXD_SSE) {
        //    if (OrderStatus == TORA_TSTP_LOS_Add) {
        //        if (m_useVec) {
        //            InsertOrderV(SecurityID, OrderNO, Price, Volume, Side);
        //        } else {
        //            InsertOrderM(SecurityID, OrderNO, Price, Volume, Side);
        //        }
        //    } else if (OrderStatus == TORA_TSTP_LOS_Delete) {
        //        if (m_useVec) {
        //            ModifyOrderV(SecurityID, 0, OrderNO, Side);
        //        } else {
        //            ModifyOrderM(SecurityID, 0, OrderNO, Side);
        //        }
        //    }
        //}
    }

    void MDL2Impl::Transaction(TTORATstpSecurityIDType SecurityID, TTORATstpExchangeIDType ExchangeID, TTORATstpLongVolumeType TradeVolume,
                               TTORATstpExecTypeType ExecType, TTORATstpLongSequenceType BuyNo, TTORATstpLongSequenceType SellNo,
                               TTORATstpPriceType TradePrice, TTORATstpTimeStampType TradeTime) {
        if (ExchangeID == TORA_TSTP_EXD_SZSE) {
            if (ExecType == TORA_TSTP_ECT_Fill) {
                if (m_useVec) {
                    ModifyOrderV(SecurityID, TradeVolume, BuyNo, TORA_TSTP_LSD_Buy);
                    ModifyOrderV(SecurityID, TradeVolume, SellNo, TORA_TSTP_LSD_Sell);
                    PostPriceV(SecurityID, TradePrice);
                } else {
                    ModifyOrderM(SecurityID, TradeVolume, BuyNo, TORA_TSTP_LSD_Buy);
                    ModifyOrderM(SecurityID, TradeVolume, SellNo, TORA_TSTP_LSD_Sell);
                    PostPriceM(SecurityID, TradePrice);
                }
            } else if (ExecType == TORA_TSTP_ECT_Cancel) {
                if (BuyNo > 0) {
                    if (m_useVec) {
                        ModifyOrderV(SecurityID, 0, BuyNo, TORA_TSTP_LSD_Buy);
                    } else {
                        ModifyOrderM(SecurityID, 0, BuyNo, TORA_TSTP_LSD_Buy);
                    }
                } else if (SellNo > 0) {
                    if (m_useVec) {
                        ModifyOrderV(SecurityID, 0, SellNo, TORA_TSTP_LSD_Sell);
                    } else {
                        ModifyOrderM(SecurityID, 0, SellNo, TORA_TSTP_LSD_Sell);
                    }
                }
            }
        }
		//else if (ExchangeID == TORA_TSTP_EXD_SSE) {
        //    if (m_useVec) {
        //        ModifyOrderV(SecurityID, TradeVolume, BuyNo, TORA_TSTP_LSD_Buy);
        //        ModifyOrderV(SecurityID, TradeVolume, SellNo, TORA_TSTP_LSD_Sell);
        //        PostPriceV(SecurityID, TradePrice);
        //    } else {
        //        ModifyOrderM(SecurityID, TradeVolume, BuyNo, TORA_TSTP_LSD_Buy);
        //        ModifyOrderM(SecurityID, TradeVolume, SellNo, TORA_TSTP_LSD_Sell);
        //        PostPriceM(SecurityID, TradePrice);
        //    }
        //}
    }

    void MDL2Impl::NGTSTick(TTORATstpSecurityIDType SecurityID, TTORATstpExchangeIDType ExchangeID, TTORATstpLTickTypeType TickType, TTORATstpLongSequenceType BuyNo,
                            TTORATstpLongSequenceType SellNo, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume,
                            TTORATstpLSideType Side, TTORATstpTimeStampType TickTime) {
		if (ExchangeID == TORA_TSTP_EXD_SSE) {
			if (TickType == TORA_TSTP_LTT_Add) {
				if (m_useVec) {
					InsertOrderV(SecurityID, Side==TORA_TSTP_LSD_Buy?BuyNo:SellNo, Price, Volume, Side);
				} else {
					InsertOrderM(SecurityID, Side==TORA_TSTP_LSD_Buy?BuyNo:SellNo, Price, Volume, Side);
				}
			} else if (TickType == TORA_TSTP_LTT_Delete) {
				if (m_useVec) {
					ModifyOrderV(SecurityID, 0, Side==TORA_TSTP_LSD_Buy?BuyNo:SellNo, Side);
				} else {
					ModifyOrderM(SecurityID, 0, Side==TORA_TSTP_LSD_Buy?BuyNo:SellNo, Side);
				}
			} else if (TickType == TORA_TSTP_LTT_Trade) {
				if (m_useVec) {
					ModifyOrderV(SecurityID, Volume, BuyNo, TORA_TSTP_LSD_Buy);
					ModifyOrderV(SecurityID, Volume, SellNo, TORA_TSTP_LSD_Sell);
					PostPriceV(SecurityID, Price);
				} else {
					ModifyOrderM(SecurityID, Volume, BuyNo, TORA_TSTP_LSD_Buy);
					ModifyOrderM(SecurityID, Volume, SellNo, TORA_TSTP_LSD_Sell);
					PostPriceM(SecurityID, Price);
				}
			}
		}
	}

    /******************************Vector******************************/
    void MDL2Impl::InsertOrderV(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType OrderNO, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpLSideType Side) {
        auto order = m_pool.Malloc<stOrder>(sizeof(stOrder));
        order->OrderNo = OrderNO;
        order->Volume = Volume;

        auto added = false;
        auto &mapOrder = Side == TORA_TSTP_LSD_Buy ? m_orderBuyV : m_orderSellV;
        auto iter = mapOrder.find(SecurityID);
        if (iter == mapOrder.end()) {
            mapOrder[SecurityID] = std::vector<stPriceOrders>();
            iter = mapOrder.find(SecurityID);
        } else if (!iter->second.empty()) {
            auto size = (int) iter->second.size();
            for (auto i = 0; i < size; i++) {
                auto nextPrice = iter->second.at(i).Price;
                auto lastPrice = iter->second.at(size - 1).Price;
                if (Side == TORA_TSTP_LSD_Buy) {
                    if (Price > nextPrice + 0.000001) {
                        stPriceOrders priceOrder = {0};
                        priceOrder.Price = Price;
                        priceOrder.Orders.emplace_back(order);
                        iter->second.insert(iter->second.begin() + i, priceOrder);
                        added = true;
                        break;
                    } else if (Price > nextPrice - 0.000001) {
                        iter->second.at(i).Orders.emplace_back(order);
                        added = true;
                        break;
                    }
                } else if (Side == TORA_TSTP_LSD_Sell) {
                    if (Price < nextPrice - 0.000001) {
                        stPriceOrders priceOrder = {0};
                        priceOrder.Price = Price;
                        priceOrder.Orders.emplace_back(order);
                        iter->second.insert(iter->second.begin() + i, priceOrder);
                        added = true;
                        break;
                    } else if (Price < nextPrice + 0.000001) {
                        iter->second.at(i).Orders.emplace_back(order);
                        added = true;
                        break;
                    }
                }
            }
        }
        if (!added) {
            stPriceOrders priceOrder = {0};
            priceOrder.Price = Price;
            priceOrder.Orders.emplace_back(order);
            iter->second.emplace_back(priceOrder);
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
                    }
                    if (Volume == 0 || (*iterOrder)->Volume <= 0) {
                        auto order = (*iterOrder);
                        iterOrder = iterVecOrder->Orders.erase(iterOrder);
                        m_pool.Free<stOrder>(order, sizeof(stOrder));
                    } else {
                        ++iterOrder;
                    }
                    nb = true;
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
        stream << "\n";
        stream << "-----------" << SecurityID << " " << GetTimeStr() << "-----------\n";

        char buffer[512] = {0};
        {
            std::vector<std::string> temp;
            auto iter = m_orderSellV.find(SecurityID);
            if (iter != m_orderSellV.end()) {
                int i = 1;
                for (auto iter1: iter->second) {
                    memset(buffer, 0, sizeof(buffer));
                    TTORATstpLongVolumeType totalVolume = 0;
                    for (auto iter2 : iter1.Orders) totalVolume += iter2->Volume;
                    int Volume = (int)totalVolume / 100;
                    if (totalVolume % 100 >= 50) Volume++;
                    sprintf(buffer, "S%d\t%.2f\t%d\t\t%d\n", i, iter1.Price, Volume, (int)iter1.Orders.size());
                    temp.emplace_back(buffer);
                    if (i++ >= 5) break;
                }
                for (auto it = temp.rbegin(); it != temp.rend(); ++it) stream << (*it);
            }
        }

        {
            std::vector<std::string> temp;
            auto iter = m_orderBuyV.find(SecurityID);
            if (iter != m_orderBuyV.end()) {
                int i = 1;
                for (auto iter1: iter->second) {
                    memset(buffer, 0, sizeof(buffer));
                    TTORATstpLongVolumeType totalVolume = 0;
                    for (auto iter2 : iter1.Orders) totalVolume += iter2->Volume;
                    int Volume = (int)totalVolume / 100;
                    if (totalVolume % 100 >= 50) Volume++;
                    sprintf(buffer, "B%d\t%.2f\t%d\t\t%d\n", i, iter1.Price, Volume, (int)iter1.Orders.size());
                    temp.emplace_back(buffer);
                    if (i++ >= 5) break;
                }
                for (auto it = temp.begin(); it != temp.end(); ++it) stream << (*it);
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
            if (iter != m_orderSellV.end()) {
                for (auto it = iter->second.begin(); it != iter->second.end(); ++it) {
                    if (!it->Orders.empty()) {
                        TTORATstpLongVolumeType Volume = 0;
                        for (auto i : it->Orders) {
                            Volume += i->Volume;
                        }
                        AskPrice1 = iter->second.begin()->Price;
                        AskVolume1 = Volume;
                        break;
                    }
                }
            }
        }

        {
            auto iter = m_orderBuyV.find(SecurityID);
            if (iter != m_orderBuyV.end()) {
                for (auto it = iter->second.begin(); it != iter->second.end(); ++it) {
                    if (!it->Orders.empty()) {
                        TTORATstpLongVolumeType Volume = 0;
                        for (auto i : it->Orders) {
                            Volume += i->Volume;
                        }
                        BidPrice1 = iter->second.begin()->Price;
                        BidVolume1 = Volume;
                        break;
                    }
                }
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
                    if (Price > curPrice + 0.000001) {
                        stPriceOrdersMap priceOrder = {0};
                        priceOrder.Price = Price;
                        priceOrder.Orders[OrderNO] = order;
                        iter->second.insert(iterOrderList, priceOrder);
                        added = true;
                        break;
                    } else if (Price > curPrice - 0.000001) {
                        iterOrderList->Orders[OrderNO] = order;
                        added = true;
                        break;
                    }
                } else if (Side == TORA_TSTP_LSD_Sell) {
                    if (Price < curPrice) {
                        stPriceOrdersMap priceOrder = {0};
                        priceOrder.Price = Price;
                        priceOrder.Orders[OrderNO] = order;
                        iter->second.insert(iterOrderList, priceOrder);
                        added = true;
                        break;
                    } else if (Price == curPrice) {
                        iterOrderList->Orders[OrderNO] = order;
                        added = true;
                        break;
                    }
                }
            }
        }
        if (!added) {
            stPriceOrdersMap priceOrder = {0};
            priceOrder.Price = Price;
            priceOrder.Orders[OrderNO] = order;
            iter->second.emplace_back(priceOrder);
        }
    }

    void MDL2Impl::ModifyOrderM(TTORATstpSecurityIDType SecurityID, TTORATstpLongVolumeType Volume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType Side) {
        auto &mapOrder = Side == TORA_TSTP_LSD_Buy ? m_orderBuyM : m_orderSellM;
        auto iter = mapOrder.find(SecurityID);
        if (iter == mapOrder.end()) return;

        for (auto iterOrderList = iter->second.begin(); iterOrderList != iter->second.end();) {
            if (iterOrderList->Orders.find(OrderNo) != iterOrderList->Orders.end()) {
                if (Volume > 0) {
                    iterOrderList->Orders[OrderNo]->Volume -= Volume;
                }
                if (Volume == 0 || iterOrderList->Orders[OrderNo]->Volume <= 0) {
                    auto tmp = iterOrderList->Orders[OrderNo];
                    iterOrderList->Orders.erase(OrderNo);
                    m_pool.Free<stOrder>(tmp, sizeof(stOrder));
                }
                if (iterOrderList->Orders.empty()) {
                    iterOrderList = iter->second.erase(iterOrderList);
                }
                break;
            } else {
                ++iterOrderList;
            }
        }
    }

    void MDL2Impl::ShowOrderBookM(TTORATstpSecurityIDType SecurityID) {
        if (m_orderBuyM.find(SecurityID) == m_orderBuyM.end() &&
            m_orderSellM.find(SecurityID) == m_orderSellM.end()) return;

        std::stringstream stream;
        stream << "\n";
        stream << "-----------" << SecurityID << " " << GetTimeStr() << "-----------\n";

        char buffer[512] = {0};
        {
            std::vector<std::string> temp;
            auto iter = m_orderSellM.find(SecurityID);
            if (iter != m_orderSellM.end()) {
                int i = 1;
                for (auto iter1: iter->second) {
                    memset(buffer, 0, sizeof(buffer));
                    TTORATstpLongVolumeType totalVolume = 0;
                    for (auto iter2 : iter1.Orders) totalVolume += iter2.second->Volume;
                    int Volume = (int)totalVolume / 100;
                    if (totalVolume % 100 >= 50) Volume++;
                    sprintf(buffer, "S%d\t%.2f\t%d\t\t%d\n", i, iter1.Price, Volume, (int)iter1.Orders.size());
                    temp.emplace_back(buffer);
                    if (i++ >= 5) break;
                }
                for (auto it = temp.rbegin(); it != temp.rend(); ++it) stream << (*it);
            }
        }
        {
            std::vector<std::string> temp;
            auto iter = m_orderBuyM.find(SecurityID);
            if (iter != m_orderBuyM.end()) {
                int i = 1;
                for (auto iter1: iter->second) {
                    memset(buffer, 0, sizeof(buffer));
                    TTORATstpLongVolumeType totalVolume = 0;
                    for (auto iter2 : iter1.Orders) totalVolume += iter2.second->Volume;
                    int Volume = (int)totalVolume / 100;
                    if (totalVolume % 100 >= 50) Volume++;
                    sprintf(buffer, "B%d\t%.2f\t%d\t\t%d\n", i, iter1.Price, Volume, (int)iter1.Orders.size());
                    temp.emplace_back(buffer);
                    if (i++ >= 5) break;
                }
                for (auto it = temp.begin(); it != temp.end(); ++it) stream << (*it);
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
            auto iter = m_orderSellM.find(SecurityID);
            if (iter != m_orderSellM.end()) {
                for (auto it = iter->second.begin(); it != iter->second.end(); ++it) {
                    if (!it->Orders.empty()) {
                        TTORATstpLongVolumeType Volume = 0;
                        for (auto i : it->Orders) {
                            Volume += i.second->Volume;
                        }
                        AskPrice1 = iter->second.begin()->Price;
                        AskVolume1 = Volume;
                        break;
                    }
                }
            }
        }

        {
            auto iter = m_orderBuyM.find(SecurityID);
            if (iter != m_orderBuyM.end()) {
                for (auto it = iter->second.begin(); it != iter->second.end(); ++it) {
                    if (!it->Orders.empty()) {
                        TTORATstpLongVolumeType Volume = 0;
                        for (auto i : it->Orders) {
                            Volume += i.second->Volume;
                        }
                        BidPrice1 = iter->second.begin()->Price;
                        BidVolume1 = Volume;
                        break;
                    }
                }
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
