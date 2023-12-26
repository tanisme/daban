#include "CApplication.h"

CApplication::CApplication(boost::asio::io_context& ioc)
        : m_ioc(ioc)
        , m_timer(m_ioc, boost::posix_time::milliseconds(1000)) {
}

CApplication::~CApplication() {
    if (m_MD) {
        m_MD->Api()->Join();
        m_MD->Api()->Release();
        delete m_MD;
    }

    if (m_TD) {
        m_TD->Api()->Join();
        m_TD->Api()->Release();
        delete m_TD;
    }
}

void CApplication::Init(std::string& watchSecurity) {
    trim(watchSecurity);
    std::vector<std::string> vtSecurity;
    Stringsplit(watchSecurity, ',', vtSecurity);
    for (auto& iter : vtSecurity) {
        auto security = m_pool.Malloc<stSecurity>(sizeof(stSecurity));
        strcpy(security->SecurityID, iter.c_str());
        int SecurityIDInt = atoi(security->SecurityID);
        m_watchSecurity[SecurityIDInt] = security;
    }
}

void CApplication::Start() {
    if (m_dataDir.length() > 0) {
        InitOrderMap(true);
        m_imitate.TestOrderBook(this, m_dataDir);
    } else {
        if (!m_TD) {
            m_TD = new PROTD::TDImpl(this);
            m_TD->Start();
        }
        m_timer.expires_from_now(boost::posix_time::milliseconds(5000));
        m_timer.async_wait(boost::bind(&CApplication::OnTime, this, boost::asio::placeholders::error));
    }
}

void CApplication::OnTime(const boost::system::error_code& error) {
    if (error) {
        if (error == boost::asio::error::operation_aborted) {
            return;
        }
    }

    if (m_TD->IsInited() && m_MD->IsInited()) {
        for (auto& iter : m_watchSecurity) {
            if (iter.second->ExchangeID == m_MD->GetExchangeID()) {
                ShowOrderBook((char*)iter.second->SecurityID);
            }
        }
    }

    m_timer.expires_from_now(boost::posix_time::milliseconds(6000));
    m_timer.async_wait(boost::bind(&CApplication::OnTime, this, boost::asio::placeholders::error));
}


double CApplication::GetOrderNoToPrice(int SecurityIDInt, PROMD::TTORATstpLongSequenceType OrderNO) {
    auto Price = 0.0;
    auto iter = m_orderNoToPrice.find(SecurityIDInt);
    if (iter != m_orderNoToPrice.end()) {
        auto it = iter->second.find(OrderNO);
        if (it != iter->second.end()) {
            Price = it->second;
        }
    }
    return Price;
}

void CApplication::AddOrderNoToPrice(int SecurityIDInt, PROMD::TTORATstpLongSequenceType OrderNO, PROMD::TTORATstpPriceType Price) {
    auto iter = m_orderNoToPrice.find(SecurityIDInt);
    if (iter != m_orderNoToPrice.end()) {
        auto it = iter->second.find(OrderNO);
        if (it != iter->second.end()) {
            printf("添加 %d 订单号 %lld 价格错误 已经存在该订单号价格 %.3f\n", SecurityIDInt, OrderNO, Price);
        }
    }
    m_orderNoToPrice[SecurityIDInt][OrderNO] = Price;
}

void CApplication::DelOrderNoPrice(int SecurityIDInt, PROMD::TTORATstpLongSequenceType OrderNO) {
    auto iter = m_orderNoToPrice.find(SecurityIDInt);
    if (iter == m_orderNoToPrice.end()) return;
    auto it = iter->second.find(OrderNO);
    if (it == iter->second.end()) return;
    iter->second.erase(it);
}

void CApplication::AddHomebestOrder(int SecurityIDInt, PROMD::TTORATstpLongSequenceType OrderNO,
                                    PROMD::TTORATstpLongVolumeType Volume, PROMD::TTORATstpLSideType Side) {
    auto iter = m_homeBaseOrder.find(SecurityIDInt);
    if (iter != m_homeBaseOrder.end()) {
        auto it = iter->second.find(OrderNO);
        if (it != iter->second.end()) {
            printf("添加 %d 最优价单 %lld 错误 已经存在!!!\n", SecurityIDInt, OrderNO);
            it->second->Volume = Volume;
            it->second->Side = Side;
            return;
        }
    }
    auto homeBestOrder = m_pool.Malloc<stHomebestOrder>(sizeof(stHomebestOrder));
    homeBestOrder->OrderNO = OrderNO;
    homeBestOrder->Volume = Volume;
    homeBestOrder->Side = Side;
    m_homeBaseOrder[SecurityIDInt][OrderNO] = homeBestOrder;
}

stHomebestOrder* CApplication::GetHomebestOrder(int SecurityIDInt, PROMD::TTORATstpLongSequenceType OrderNO) {
    auto iter = m_homeBaseOrder.find(SecurityIDInt);
    if (iter == m_homeBaseOrder.end()) return nullptr;
    auto it = iter->second.find(OrderNO);
    if (it == iter->second.end()) return nullptr;
    return it->second;
}

void CApplication::DelHomebestOrder(int SecurityIDInt, PROMD::TTORATstpLongSequenceType OrderNO) {
    auto iter = m_homeBaseOrder.find(SecurityIDInt);
    if (iter == m_homeBaseOrder.end()) return;
    auto it = iter->second.find(OrderNO);
    if (it == iter->second.end()) return;
    auto ptr = it->second;
    iter->second.erase(it);
    m_pool.Free<stHomebestOrder>(ptr, sizeof(stHomebestOrder));
}

int CApplication::GetHTLPriceIndex(int SecurityIDInt, PROMD::TTORATstpPriceType Price, PROMD::TTORATstpTradeBSFlagType Side) {
    auto iter = m_marketSecurity.find(SecurityIDInt);
    if (iter == m_marketSecurity.end()) return -1;

    auto tempIndex = -2.0;
    if (Side == PROMD::TORA_TSTP_LSD_Buy) {
        auto diffPrice = iter->second->UpperLimitPrice - Price;
        tempIndex = diffPrice / 0.01;
    } else {
        auto diffPrice = Price - iter->second->LowerLimitPrice;
        tempIndex = diffPrice / 0.01;
    }

    auto priceIndex = int(tempIndex);
    auto value = tempIndex - priceIndex;
    if (value >= 0.5) {
        priceIndex++;
    }

    auto TotalIndex = GetTotalIndex(iter->second->UpperLimitPrice, iter->second->LowerLimitPrice);
    if (priceIndex >= TotalIndex || priceIndex < 0) {
        priceIndex = -1;
    }
    return priceIndex;
}

void CApplication::AddOrderPriceIndex(int SecurityIDInt, int priceIndex, PROMD::TTORATstpTradeBSFlagType Side) {
    auto& orderIndex = Side==PROMD::TORA_TSTP_LSD_Buy?m_buyPriceIndex:m_sellPriceIndex;
    orderIndex[SecurityIDInt].insert(priceIndex);
}

void CApplication::DelOrderPriceIndex(int SecurityIDInt, int priceIndex, PROMD::TTORATstpTradeBSFlagType Side) {
    auto& orderIndex = Side==PROMD::TORA_TSTP_LSD_Buy?m_buyPriceIndex:m_sellPriceIndex;
    auto iter = orderIndex.find(SecurityIDInt);
    if (iter == orderIndex.end()) return;
    auto it = iter->second.find(priceIndex);
    if (it == iter->second.end()) return;
    iter->second.erase(it);
    if (iter->second.empty()) {
        orderIndex.erase(iter);
    }
}

void CApplication::InsertOrderN(int SecurityIDInt, PROMD::TTORATstpLongSequenceType OrderNO, PROMD::TTORATstpPriceType Price,
                                PROMD::TTORATstpLongVolumeType Volume, PROMD::TTORATstpLSideType Side) {
    auto priceIndex = GetHTLPriceIndex(SecurityIDInt, Price, Side);
    if (priceIndex < 0) {
        //printf("Insert GetHTLPriceIndex SecurityID:%d Price:%.2f Side:%c\n", SecurityIDInt, Price, Side);
        return;
    }

    AddOrderNoToPrice(SecurityIDInt, OrderNO, Price);
    AddOrderPriceIndex(SecurityIDInt, priceIndex, Side);

    auto order = m_pool.Malloc<stOrder>(sizeof(stOrder));
    order->OrderNo = OrderNO;
    order->Volume = Volume;
    auto& mapOrder = Side==PROMD::TORA_TSTP_LSD_Buy?m_orderBuyN:m_orderSellN;
    mapOrder.at(SecurityIDInt).at(priceIndex).Orders.emplace_back(order);
}

int CApplication::ModifyOrderN(int SecurityIDInt, PROMD::TTORATstpLongVolumeType Volume,
                               PROMD::TTORATstpLongSequenceType OrderNo, PROMD::TTORATstpTradeBSFlagType Side) {
    auto Price = GetOrderNoToPrice(SecurityIDInt, OrderNo);
    if (Price < 0.000001) {
        //printf("Modify GetOrderNoToPrice SecurityID:%d OrderNO:%lld\n", SecurityIDInt, OrderNo);
        return -1;
    }
    auto priceIndex = GetHTLPriceIndex(SecurityIDInt, Price, Side);
    if (priceIndex < 0) {
        printf("Modify GetHTLPriceIndex SecurityID:%d Price:%.2f Side:%c\n", SecurityIDInt, Price, Side);
        return -1;
    }

    auto& curPriceIndex = Side==PROMD::TORA_TSTP_LSD_Buy?m_buyPriceIndex:m_sellPriceIndex;
    auto priceIndexIter = curPriceIndex.find(SecurityIDInt);
    if (priceIndexIter == curPriceIndex.end()) {
        printf("Modify 111\n");
        return priceIndex;
    }

    if (priceIndexIter->second.find(priceIndex) == priceIndexIter->second.end()) {
        printf("Modify 222 priceIndex = %d OrderNO:%lld\n", priceIndex, OrderNo);
        return priceIndex;
    }

    auto& mapOrder = Side==PROMD::TORA_TSTP_LSD_Buy?m_orderBuyN:m_orderSellN;
    std::vector<stOrder*>::iterator iter;
    auto erfen = true;
    if (!erfen){
        iter = std::find_if(mapOrder.at(SecurityIDInt).at(priceIndex).Orders.begin(),
                            mapOrder.at(SecurityIDInt).at(priceIndex).Orders.end(), [&](stOrder* order) {
                    return order && order->OrderNo == OrderNo;
                } );
    } else {
        std::vector<PROMD::TTORATstpLongSequenceType> OrderNoVec;
        for (auto& it : mapOrder.at(SecurityIDInt).at(priceIndex).Orders) OrderNoVec.push_back(it->OrderNo);
        auto idx = FindOrderNo(OrderNoVec, OrderNo);
        if (idx < 0) return priceIndex;
        iter = mapOrder.at(SecurityIDInt).at(priceIndex).Orders.begin() + idx;
    }

    if (iter == mapOrder.at(SecurityIDInt).at(priceIndex).Orders.end()) {
        return priceIndex;
    }

    if (Volume > 0) {
        (*iter)->Volume -= Volume;
    }

    auto rmPriceIndex = false;
    if (Volume == 0 || (*iter)->Volume <= 0) {
        DelOrderNoPrice(SecurityIDInt, OrderNo);
        auto order = *iter;
        mapOrder.at(SecurityIDInt).at(priceIndex).Orders.erase(iter);
        m_pool.Free<stOrder>(order, sizeof(stOrder));
        rmPriceIndex = mapOrder.at(SecurityIDInt).at(priceIndex).Orders.empty();
    }

    if (rmPriceIndex) {
        DelOrderPriceIndex(SecurityIDInt, priceIndex, Side);
    }
    return priceIndex;
}

void CApplication::DeleteOrderN(int SecurityIDInt, int priceIndex,
                                PROMD::TTORATstpLongSequenceType OrderNo, PROMD::TTORATstpTradeBSFlagType Side) {
    if (priceIndex < 0) return;
    auto& curPriceIndex = Side==PROMD::TORA_TSTP_LSD_Buy?m_buyPriceIndex:m_sellPriceIndex;
    auto priceIndexIter = curPriceIndex.find(SecurityIDInt);
    if (priceIndexIter == curPriceIndex.end()) return;

    std::vector<int> toRemovePriceIndex;
    auto& mapOrder = Side==PROMD::TORA_TSTP_LSD_Buy?m_orderBuyN:m_orderSellN;
    for (auto it = priceIndexIter->second.begin(); it != priceIndexIter->second.end(); ++it) {
        auto curPriceIndex = *it;
        if (curPriceIndex == priceIndex) {
            auto iter = mapOrder.at(SecurityIDInt).at((curPriceIndex)).Orders.begin();
            for (;iter != mapOrder.at(SecurityIDInt).at((curPriceIndex)).Orders.end();) {
                auto order = (*iter);
                if (order->OrderNo < OrderNo) {
                    DelOrderNoPrice(SecurityIDInt, order->OrderNo);
                    iter = mapOrder.at(SecurityIDInt).at((curPriceIndex)).Orders.erase(iter);
                    m_pool.Free<stOrder>(order, sizeof(stOrder));
                } else {
                    break;
                }
            }
            if (mapOrder.at(SecurityIDInt).at((curPriceIndex)).Orders.empty()) {
                toRemovePriceIndex.push_back(curPriceIndex);
            }
        } else {
            if (curPriceIndex < priceIndex) {
                auto iter = mapOrder.at(SecurityIDInt).at(curPriceIndex).Orders.begin();
                for (;iter != mapOrder.at(SecurityIDInt).at(curPriceIndex).Orders.end(); ++iter) {
                    auto order = (*iter);
                    DelOrderNoPrice(SecurityIDInt, order->OrderNo);
                    m_pool.Free<stOrder>(order, sizeof(stOrder));
                }
                mapOrder.at(SecurityIDInt).at(priceIndex).Orders.clear();
                toRemovePriceIndex.push_back(curPriceIndex);
            } else {
                break;
            }
        }
    }

    for (auto it : toRemovePriceIndex) {
        DelOrderPriceIndex(SecurityIDInt, it, Side);
    }
}

/***************************************TD***************************************/
void CApplication::TDOnRspQrySecurity(PROTD::CTORATstpSecurityField &Security) {
    if ((m_isSHExchange && Security.ExchangeID == PROMD::TORA_TSTP_EXD_SSE) ||
            (!m_isSHExchange && Security.ExchangeID == PROMD::TORA_TSTP_EXD_SZSE)) {
        int SecurityIDInt = atoi(Security.SecurityID);
        auto it = m_marketSecurity.find(SecurityIDInt);
        if (it == m_marketSecurity.end()) {
            auto security = m_pool.Malloc<stSecurity>(sizeof(stSecurity));
            if (security) {
                strcpy(security->SecurityID, Security.SecurityID);
                security->ExchangeID = Security.ExchangeID;
                security->UpperLimitPrice = Security.UpperLimitPrice;
                security->LowerLimitPrice = Security.LowerLimitPrice;
                m_marketSecurity[SecurityIDInt] = security;
            }
            if (m_watchSecurity.find(SecurityIDInt) != m_watchSecurity.end()) {
                strcpy(m_watchSecurity[SecurityIDInt]->SecurityID, Security.SecurityID);
                m_watchSecurity[SecurityIDInt]->ExchangeID = Security.ExchangeID;
            }
        }
    }
}

void CApplication::TDOnInitFinished() {
    printf("CApplication::TDOnInitFinished\n");

    if (m_MD) return;
    InitOrderMap(false);
    PROMD::TTORATstpExchangeIDType ExchangeID = PROMD::TORA_TSTP_EXD_SZSE;
    if (m_isSHExchange) ExchangeID = PROMD::TORA_TSTP_EXD_SSE;
    m_MD = new PROMD::MDL2Impl(this, ExchangeID);
    m_MD->Start(m_isTest);
}

/***************************************MD***************************************/
void CApplication::InitOrderMap(bool isPart) {
    m_orderBuyN.resize(999999);
    m_orderSellN.resize(999999);
    auto& initSecurity = m_marketSecurity;
    if (isPart) {
        for (auto& it : m_watchSecurity) {
            it.second->UpperLimitPrice = 40;
            it.second->LowerLimitPrice = 20;
        }
        initSecurity = m_watchSecurity;
    }
    for (auto& it : initSecurity) {
        auto SecurityIDInt = atoi(it.second->SecurityID);
        auto TotalIndex = GetTotalIndex(it.second->UpperLimitPrice, it.second->LowerLimitPrice);
        m_orderBuyN.at(SecurityIDInt).resize(TotalIndex);
        m_orderSellN.at(SecurityIDInt).resize(TotalIndex);
        for (auto i = 0; i < TotalIndex; ++i) {
            m_orderBuyN.at(SecurityIDInt).at(i).Price = it.second->UpperLimitPrice - i * 0.01;
            m_orderSellN.at(SecurityIDInt).at(i).Price = it.second->LowerLimitPrice + i * 0.01;
        }
    }
}

void CApplication::MDOnInitFinished(PROMD::TTORATstpExchangeIDType ExchangeID) {
    auto cnt = 0;
    for (auto &iter: m_marketSecurity) {
        if (ExchangeID == iter.second->ExchangeID) {
            cnt++;
            PROMD::TTORATstpSecurityIDType SecurityID = {0};
            strncpy(SecurityID, iter.second->SecurityID, sizeof(SecurityID));
            char *security_arr[1];
            security_arr[0] = SecurityID;
            if (iter.second->ExchangeID == PROMD::TORA_TSTP_EXD_SSE) {
                m_MD->Api()->SubscribeNGTSTick(security_arr, 1, ExchangeID);
            } else {
                m_MD->Api()->SubscribeOrderDetail(security_arr, 1, ExchangeID);
                m_MD->Api()->SubscribeTransaction(security_arr, 1, ExchangeID);
            }
        }
    }
    printf("CApplication::MDOnInitFinished %s subscribe:%d total:%d\n",
           PROMD::MDL2Impl::GetExchangeName(ExchangeID), cnt, (int)m_marketSecurity.size());
}

void CApplication::MDOnRtnOrderDetail(PROMD::CTORATstpLev2OrderDetailField &OrderDetail) {
    int SecurityIDInt = atoi(OrderDetail.SecurityID);
    m_notifyCount[SecurityIDInt]++;
    if (OrderDetail.OrderType == PROMD::TORA_TSTP_LOT_Market) return;
    if (OrderDetail.ExchangeID != PROMD::TORA_TSTP_EXD_SZSE) return;
    if (OrderDetail.Side != PROMD::TORA_TSTP_LSD_Buy && OrderDetail.Side != PROMD::TORA_TSTP_LSD_Sell) return;

    if (OrderDetail.OrderType == PROMD::TORA_TSTP_LOT_HomeBest) {
        AddHomebestOrder(SecurityIDInt, OrderDetail.OrderNO, OrderDetail.Volume, OrderDetail.Side);
        return;
    }
    InsertOrderN(SecurityIDInt, OrderDetail.OrderNO, OrderDetail.Price, OrderDetail.Volume, OrderDetail.Side);
}

void CApplication::MDOnRtnTransaction(PROMD::CTORATstpLev2TransactionField &Transaction) {
    int SecurityIDInt = atoi(Transaction.SecurityID);
    m_notifyCount[SecurityIDInt]++;
    if (Transaction.ExchangeID != PROMD::TORA_TSTP_EXD_SZSE) return;
    if (Transaction.ExecType == PROMD::TORA_TSTP_ECT_Fill) {
        auto homeBestBuyOrder = GetHomebestOrder(SecurityIDInt, Transaction.BuyNo);
        if (homeBestBuyOrder) {
            homeBestBuyOrder->Volume -= Transaction.TradeVolume;
            if (homeBestBuyOrder->Volume > 0) {
                InsertOrderN(SecurityIDInt, homeBestBuyOrder->OrderNO, Transaction.TradePrice, homeBestBuyOrder->Volume, homeBestBuyOrder->Side);
            }
            DelHomebestOrder(SecurityIDInt, Transaction.BuyNo);
        } else {
            auto priceIndex = ModifyOrderN(SecurityIDInt, Transaction.TradeVolume, Transaction.BuyNo, PROMD::TORA_TSTP_LSD_Buy);
            DeleteOrderN(SecurityIDInt, priceIndex, Transaction.BuyNo, PROMD::TORA_TSTP_LSD_Buy);
        }
        auto homeBestSellOrder = GetHomebestOrder(SecurityIDInt, Transaction.SellNo);
        if (homeBestSellOrder) {
            homeBestSellOrder->Volume -= Transaction.TradeVolume;
            if (homeBestSellOrder->Volume > 0) {
                InsertOrderN(SecurityIDInt, homeBestSellOrder->OrderNO, Transaction.TradePrice, homeBestSellOrder->Volume, homeBestSellOrder->Side);
            }
            DelHomebestOrder(SecurityIDInt, Transaction.SellNo);
        } else {
            auto priceIndex = ModifyOrderN(SecurityIDInt, Transaction.TradeVolume, Transaction.SellNo, PROMD::TORA_TSTP_LSD_Sell);
            DeleteOrderN(SecurityIDInt, priceIndex, Transaction.SellNo, PROMD::TORA_TSTP_LSD_Sell);
        }
    } else if (Transaction.ExecType == PROMD::TORA_TSTP_ECT_Cancel) {
        if (Transaction.BuyNo > 0) {
            ModifyOrderN(SecurityIDInt, 0, Transaction.BuyNo, PROMD::TORA_TSTP_LSD_Buy);
        } else if (Transaction.SellNo > 0) {
            ModifyOrderN(SecurityIDInt, 0, Transaction.SellNo, PROMD::TORA_TSTP_LSD_Sell);
        }
    }
}

void CApplication::MDOnRtnNGTSTick(PROMD::CTORATstpLev2NGTSTickField &Tick) {
    int SecurityIDInt = atoi(Tick.SecurityID);
    m_notifyCount[SecurityIDInt]++;

    if (Tick.ExchangeID != PROMD::TORA_TSTP_EXD_SSE) return;
    if (Tick.Side != PROMD::TORA_TSTP_LSD_Buy && Tick.Side != PROMD::TORA_TSTP_LSD_Sell) return;
    if (Tick.TickType == PROMD::TORA_TSTP_LTT_Add) {
        InsertOrderN(SecurityIDInt, Tick.Side==PROMD::TORA_TSTP_LSD_Buy?Tick.BuyNo:Tick.SellNo, Tick.Price, Tick.Volume, Tick.Side);
    } else if (Tick.TickType == PROMD::TORA_TSTP_LTT_Delete) {
        ModifyOrderN(SecurityIDInt, 0, Tick.Side==PROMD::TORA_TSTP_LSD_Buy?Tick.BuyNo:Tick.SellNo, Tick.Side);
    } else if (Tick.TickType == PROMD::TORA_TSTP_LTT_Trade) {
        {
            auto priceIndex = ModifyOrderN(SecurityIDInt, Tick.Volume, Tick.BuyNo, PROMD::TORA_TSTP_LSD_Buy);
            DeleteOrderN(SecurityIDInt, priceIndex, Tick.BuyNo, PROMD::TORA_TSTP_LSD_Buy);
        }
        {
            auto priceIndex = ModifyOrderN(SecurityIDInt, Tick.Volume, Tick.SellNo, PROMD::TORA_TSTP_LSD_Sell);
            DeleteOrderN(SecurityIDInt, priceIndex, Tick.SellNo, PROMD::TORA_TSTP_LSD_Sell);
        }
    }
}

void CApplication::ShowOrderBook(PROMD::TTORATstpSecurityIDType SecurityID) {
    int SecurityIDInt = atoi(SecurityID);
    int count = 0;
    {
        auto iter = m_notifyCount.find(SecurityIDInt);
        if (iter != m_notifyCount.end()) count = m_notifyCount[SecurityIDInt];
    }

    std::stringstream stream;
    stream << "\n";
    stream << "-----------" << SecurityID << " " << count << " " << GetTimeStr() << "-----------\n";

    char buffer[512] = {0};
    {
        std::vector<std::string> temp;
        int i = 1;
        auto iter = m_sellPriceIndex.find(SecurityIDInt);
        if (iter != m_sellPriceIndex.end()) {
            for (auto it : iter->second) {
                auto priceIndex = it;
                auto orders = m_orderSellN.at(SecurityIDInt).at(priceIndex);
                auto totalVolume = 0ll;
                memset(buffer, 0, sizeof(buffer));
                for (auto iter1 : orders.Orders) {
                    totalVolume += iter1->Volume;
                }
                sprintf(buffer, "S%d\t%.2f\t%lld\t\t%d\n", i, orders.Price, totalVolume, (int)orders.Orders.size());
                temp.emplace_back(buffer);
                if (i++ >= 5) break;
            }
        }
        for (auto it = temp.rbegin(); it != temp.rend(); ++it) stream << (*it);
    }
    {
        std::vector<std::string> temp;
        int i = 1;
        auto iter = m_buyPriceIndex.find(SecurityIDInt);
        if (iter != m_buyPriceIndex.end()) {
            for (auto it : iter->second) {
                auto priceIndex = it;
                auto orders = m_orderBuyN.at(SecurityIDInt).at(priceIndex);
                auto totalVolume = 0ll;
                memset(buffer, 0, sizeof(buffer));
                for (auto iter1 : orders.Orders) {
                    totalVolume += iter1->Volume;
                }
                sprintf(buffer, "B%d\t%.2f\t%lld\t\t%d\n", i, orders.Price, totalVolume, (int)orders.Orders.size());
                temp.emplace_back(buffer);
                if (i++ >= 5) break;
            }
        }
        for (auto it = temp.begin(); it != temp.end(); ++it) stream << (*it);
    }

    printf("%s\n", stream.str().c_str());
}

