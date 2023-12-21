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

    for (auto& iter : m_watchSecurity) {
        //ShowOrderBook((char*)iter.second->SecurityID);
    }

    m_timer.expires_from_now(boost::posix_time::milliseconds(60000));
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
    auto value = int(tempIndex * 10) % 10;
    if (value >= 5) {
        priceIndex++;
    }
    if (priceIndex >= iter->second->TotalIndex || priceIndex < 0) {
        priceIndex = -1;
    }
    return priceIndex;
}

void CApplication::AddOrderPriceIndex(int SecurityIDInt, int priceIndex, PROMD::TTORATstpTradeBSFlagType Side) {
    auto& orderIndex = Side==PROMD::TORA_TSTP_LSD_Buy?m_buyPriceIndex:m_sellPriceIndex;
    auto iter = orderIndex.find(SecurityIDInt);
    if (iter == orderIndex.end()) {
        orderIndex[SecurityIDInt][priceIndex] = 1;
    } else {
        auto it = iter->second.find(priceIndex);
        if (it == iter->second.end()) {
            orderIndex[SecurityIDInt][priceIndex] = 1;
        } else {
            it->second++;
        }
    }
}

void CApplication::ModOrderPriceIndex(int SecurityIDInt, int priceIndex, PROMD::TTORATstpTradeBSFlagType Side) {
    auto& orderIndex = Side==PROMD::TORA_TSTP_LSD_Buy?m_buyPriceIndex:m_sellPriceIndex;
    auto iter = orderIndex.find(SecurityIDInt);
    if (iter == orderIndex.end()) return;
    auto it = iter->second.find(priceIndex);
    if (it == iter->second.end()) return;
    it->second -= 1;
    if (it->second <= 0) {
        iter->second.erase(it);
    }
    if (iter->second.empty()) {
        orderIndex.erase(iter);
    }
}

void CApplication::InsertOrderN(int SecurityIDInt, PROMD::TTORATstpLongSequenceType OrderNO, PROMD::TTORATstpPriceType Price,
                                PROMD::TTORATstpLongVolumeType Volume, PROMD::TTORATstpLSideType Side) {
    auto priceIndex = GetHTLPriceIndex(SecurityIDInt, Price, Side);
    if (priceIndex < 0) return;

    AddOrderNoToPrice(SecurityIDInt, OrderNO, Price);
    AddOrderPriceIndex(SecurityIDInt, priceIndex, Side);

    auto order = m_pool.Malloc<stOrder>(sizeof(stOrder));
    order->OrderNo = OrderNO;
    order->Volume = Volume;
    auto& mapOrder = Side == PROMD::TORA_TSTP_LSD_Buy?m_orderBuyN:m_orderSellN;
    mapOrder.at(SecurityIDInt).at(priceIndex).Orders.emplace_back(order);
}

void CApplication::DeleteOrderN(int SecurityIDInt, PROMD::TTORATstpLongVolumeType Volume,
                                PROMD::TTORATstpLongSequenceType OrderNo, PROMD::TTORATstpTradeBSFlagType Side) {
    auto Price = GetOrderNoToPrice(SecurityIDInt, OrderNo);
    if (Price < 0.000001) return;
    auto priceIndex = GetHTLPriceIndex(SecurityIDInt, Price, Side);
    if (priceIndex < 0) return;
    auto& curPriceIndex = Side==PROMD::TORA_TSTP_LSD_Buy?m_buyPriceIndex:m_sellPriceIndex;
    auto priceIndexIter = curPriceIndex.find(SecurityIDInt);
    if (priceIndexIter == curPriceIndex.end()) return;

    std::vector<int> toRemovePriceIndex;
    auto& mapOrder = Side==PROMD::TORA_TSTP_LSD_Buy?m_orderBuyN:m_orderSellN;
    for (auto it = priceIndexIter->second.begin(); it != priceIndexIter->second.end(); ++it) {
        if (it->first == priceIndex) {
            auto iter = mapOrder.at(SecurityIDInt).at(it->first).Orders.begin();
            for (;iter != mapOrder.at(SecurityIDInt).at(it->first).Orders.end();) {
                auto order = (*iter);
                if (order->OrderNo == OrderNo) {
                    DelOrderNoPrice(SecurityIDInt, order->OrderNo);
                    toRemovePriceIndex.push_back(priceIndex);
                    iter = mapOrder.at(SecurityIDInt).at(priceIndex).Orders.erase(iter);
                    m_pool.Free<stOrder>(order, sizeof(stOrder));
                } else {
                    ++iter;
                }
            }
        } else {
            if (it->first > priceIndex) break;
        }
    }
    for (auto it : toRemovePriceIndex) {
        ModOrderPriceIndex(SecurityIDInt, it, Side);
    }
}

void CApplication::ModifyOrderN(int SecurityIDInt, PROMD::TTORATstpLongVolumeType Volume,
                                PROMD::TTORATstpLongSequenceType OrderNo, PROMD::TTORATstpTradeBSFlagType Side) {
    auto Price = GetOrderNoToPrice(SecurityIDInt, OrderNo);
    if (Price < 0.000001) return;
    auto priceIndex = GetHTLPriceIndex(SecurityIDInt, Price, Side);
    if (priceIndex < 0) return;
    auto& curPriceIndex = Side==PROMD::TORA_TSTP_LSD_Buy?m_buyPriceIndex:m_sellPriceIndex;
    auto priceIndexIter = curPriceIndex.find(SecurityIDInt);
    if (priceIndexIter == curPriceIndex.end()) return;

    std::vector<int> toRemovePriceIndex;
    auto& mapOrder = Side==PROMD::TORA_TSTP_LSD_Buy?m_orderBuyN:m_orderSellN;
    for (auto it = priceIndexIter->second.begin(); it != priceIndexIter->second.end(); ++it) {
        if (it->first == priceIndex) {
            auto iter = mapOrder.at(SecurityIDInt).at(it->first).Orders.begin();
            for (;iter != mapOrder.at(SecurityIDInt).at(it->first).Orders.end();) {
                auto order = (*iter);
                if (order->OrderNo == OrderNo) {
                    if (Volume > 0) {
                        order->Volume -= Volume;
                    }
                    if (Volume == 0 || order->Volume <= 0) {
                        DelOrderNoPrice(SecurityIDInt, order->OrderNo);
                        toRemovePriceIndex.push_back(it->first);
                        iter = mapOrder.at(SecurityIDInt).at(it->first).Orders.erase(iter);
                        m_pool.Free<stOrder>(order, sizeof(stOrder));
                    } else {
                        ++iter;
                    }
                } else if (order->OrderNo < OrderNo) {
                    //if (Volume > 0) {
                    //    DelOrderNoPrice(SecurityIDInt, order->OrderNo);
                    //    iter = mapOrder.at(SecurityIDInt).at(it->first).Orders.erase(iter);
                    //    m_pool.Free<stOrder>(order, sizeof(stOrder));
                    //    toRemovePriceIndex.push_back(it->first);
                    //} else {
                        ++iter;
                    //}
                } else {
                    ++iter;
                }
            }
        } else {
            if (it->first < priceIndex) {
                //if (Volume > 0) {
                //    auto iter = mapOrder.at(SecurityIDInt).at(it->first).Orders.begin();
                //    for (;iter != mapOrder.at(SecurityIDInt).at(it->first).Orders.end(); ++iter) {
                //        auto order = (*iter);
                //        DelOrderNoPrice(SecurityIDInt, order->OrderNo);
                //        m_pool.Free<stOrder>(order, sizeof(stOrder));
                //    }
                //    mapOrder.at(SecurityIDInt).at(priceIndex).Orders.clear();
                //    toRemovePriceIndex.push_back(it->first);
                //}
            } else {
                break;
            }
        }
    }

    for (auto it : toRemovePriceIndex) {
        ModOrderPriceIndex(SecurityIDInt, it, Side);
    }
}

/***************************************TD***************************************/
void CApplication::TDOnRspQrySecurity(PROTD::CTORATstpSecurityField &Security) {
    int SecurityIDInt = atoi(Security.SecurityID);
    auto it = m_marketSecurity.find(SecurityIDInt);
    if (it == m_marketSecurity.end()) {
        auto security = m_pool.Malloc<stSecurity>(sizeof(stSecurity));
        if (security) {
            strcpy(security->SecurityID, Security.SecurityID);
            security->ExchangeID = Security.ExchangeID;
            security->UpperLimitPrice = Security.UpperLimitPrice;
            security->LowerLimitPrice = Security.LowerLimitPrice;

            auto diffPrice = security->UpperLimitPrice - security->LowerLimitPrice;
            auto tempIndex = diffPrice / 0.01;
            auto totalIndex = int(tempIndex) + 1;
            if (int(tempIndex * 10) % 10 >= 5) {
                totalIndex += 1;
            }
            security->TotalIndex = totalIndex;
            m_marketSecurity[SecurityIDInt] = security;
        }
        if (m_watchSecurity.find(SecurityIDInt) != m_watchSecurity.end()) {
            strcpy(m_watchSecurity[SecurityIDInt]->SecurityID, Security.SecurityID);
            m_watchSecurity[SecurityIDInt]->ExchangeID = Security.ExchangeID;
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
            it.second->UpperLimitPrice = 20;
            it.second->LowerLimitPrice = 10;
            it.second->TotalIndex = int((it.second->UpperLimitPrice - it.second->LowerLimitPrice) / 0.01) + 1;
        }
        initSecurity = m_watchSecurity;
    }
    for (auto& it : initSecurity) {
        auto SecurityIDInt = atoi(it.second->SecurityID);
        m_orderBuyN.at(SecurityIDInt).resize(it.second->TotalIndex);
        m_orderSellN.at(SecurityIDInt).resize(it.second->TotalIndex);
        for (auto i = 0; i < it.second->TotalIndex; ++i) {
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
    if (OrderDetail.OrderType == PROMD::TORA_TSTP_LOT_Market) return;
    if (OrderDetail.Side != PROMD::TORA_TSTP_LSD_Buy && OrderDetail.Side != PROMD::TORA_TSTP_LSD_Sell) return;

    int SecurityIDInt = atoi(OrderDetail.SecurityID);
    if (OrderDetail.ExchangeID == PROMD::TORA_TSTP_EXD_SZSE) {
        if (OrderDetail.OrderType == PROMD::TORA_TSTP_LOT_HomeBest) {
            AddHomebestOrder(SecurityIDInt, OrderDetail.OrderNO, OrderDetail.Volume, OrderDetail.Side);
            return;
        }
        InsertOrderN(SecurityIDInt, OrderDetail.OrderNO, OrderDetail.Price, OrderDetail.Volume, OrderDetail.Side);
    }
    else if (OrderDetail.ExchangeID == PROMD::TORA_TSTP_EXD_SSE) {
        if (OrderDetail.OrderStatus == PROMD::TORA_TSTP_LOS_Add) {
            InsertOrderN(SecurityIDInt, OrderDetail.OrderNO, OrderDetail.Price, OrderDetail.Volume, OrderDetail.Side);
        } else if (OrderDetail.OrderStatus == PROMD::TORA_TSTP_LOS_Delete) {
            //ModifyOrderN(SecurityIDInt, 0, OrderDetail.OrderNO, OrderDetail.Side);
            DeleteOrderN(SecurityIDInt, 0, OrderDetail.OrderNO, OrderDetail.Side);
        }
    }
}

void CApplication::MDOnRtnTransaction(PROMD::CTORATstpLev2TransactionField &Transaction) {
    int SecurityIDInt = atoi(Transaction.SecurityID);
    if (Transaction.ExchangeID == PROMD::TORA_TSTP_EXD_SZSE) {
        if (Transaction.ExecType == PROMD::TORA_TSTP_ECT_Fill) {
            auto homeBestBuyOrder = GetHomebestOrder(SecurityIDInt, Transaction.BuyNo);
            if (homeBestBuyOrder) {
                homeBestBuyOrder->Volume -= Transaction.TradeVolume;
                if (homeBestBuyOrder->Volume > 0) {
                    InsertOrderN(SecurityIDInt, homeBestBuyOrder->OrderNO, Transaction.TradePrice, homeBestBuyOrder->Volume, homeBestBuyOrder->Side);
                }
                DelHomebestOrder(SecurityIDInt, Transaction.BuyNo);
            } else {
                ModifyOrderN(SecurityIDInt, Transaction.TradeVolume, Transaction.BuyNo, PROMD::TORA_TSTP_LSD_Buy);
            }
            auto homeBestSellOrder = GetHomebestOrder(SecurityIDInt, Transaction.SellNo);
            if (homeBestSellOrder) {
                homeBestSellOrder->Volume -= Transaction.TradeVolume;
                if (homeBestSellOrder->Volume > 0) {
                    InsertOrderN(SecurityIDInt, homeBestSellOrder->OrderNO, Transaction.TradePrice, homeBestSellOrder->Volume, homeBestSellOrder->Side);
                }
                DelHomebestOrder(SecurityIDInt, Transaction.SellNo);
            } else {
                ModifyOrderN(SecurityIDInt, Transaction.TradeVolume, Transaction.SellNo, PROMD::TORA_TSTP_LSD_Sell);
            }
        } else if (Transaction.ExecType == PROMD::TORA_TSTP_ECT_Cancel) {
            if (Transaction.BuyNo > 0) {
                //ModifyOrderN(SecurityIDInt, 0, Transaction.BuyNo, PROMD::TORA_TSTP_LSD_Buy);
                DeleteOrderN(SecurityIDInt, 0, Transaction.BuyNo, PROMD::TORA_TSTP_LSD_Buy);
            } else if (Transaction.SellNo > 0) {
                //ModifyOrderN(SecurityIDInt, 0, Transaction.SellNo, PROMD::TORA_TSTP_LSD_Sell);
                DeleteOrderN(SecurityIDInt, 0, Transaction.SellNo, PROMD::TORA_TSTP_LSD_Sell);
            }
        }
    }
    else if (Transaction.ExchangeID == PROMD::TORA_TSTP_EXD_SSE) {
        ModifyOrderN(SecurityIDInt, Transaction.TradeVolume, Transaction.BuyNo, PROMD::TORA_TSTP_LSD_Buy);
        ModifyOrderN(SecurityIDInt, Transaction.TradeVolume, Transaction.SellNo, PROMD::TORA_TSTP_LSD_Sell);
    }
}

void CApplication::MDOnRtnNGTSTick(PROMD::CTORATstpLev2NGTSTickField &Tick) {
    if (Tick.ExchangeID != PROMD::TORA_TSTP_EXD_SSE) return;
    if (Tick.Side != PROMD::TORA_TSTP_LSD_Buy && Tick.Side != PROMD::TORA_TSTP_LSD_Sell) return;

    int SecurityIDInt = atoi(Tick.SecurityID);
    if (Tick.TickType == PROMD::TORA_TSTP_LTT_Add) {
        InsertOrderN(SecurityIDInt, Tick.Side==PROMD::TORA_TSTP_LSD_Buy?Tick.BuyNo:Tick.SellNo, Tick.Price, Tick.Volume, Tick.Side);
    } else if (Tick.TickType == PROMD::TORA_TSTP_LTT_Delete) {
        //ModifyOrderN(SecurityIDInt, 0, Tick.Side==PROMD::TORA_TSTP_LSD_Buy?Tick.BuyNo:Tick.SellNo, Tick.Side);
        DeleteOrderN(SecurityIDInt, 0, Tick.Side==PROMD::TORA_TSTP_LSD_Buy?Tick.BuyNo:Tick.SellNo, Tick.Side);
    } else if (Tick.TickType == PROMD::TORA_TSTP_LTT_Trade) {
        ModifyOrderN(SecurityIDInt, Tick.Volume, Tick.BuyNo, PROMD::TORA_TSTP_LSD_Buy);
        ModifyOrderN(SecurityIDInt, Tick.Volume, Tick.SellNo, PROMD::TORA_TSTP_LSD_Sell);
    }
}

void CApplication::ShowOrderBook(PROMD::TTORATstpSecurityIDType SecurityID) {
    int SecurityIDInt = atoi(SecurityID);

    std::stringstream stream;
    stream << "\n";
    stream << "-----------" << SecurityID << " " << GetTimeStr() << "-----------\n";

    char buffer[512] = {0};
    {
        std::vector<std::string> temp;
        int i = 1;
        auto iter = m_sellPriceIndex.find(SecurityIDInt);
        if (iter != m_sellPriceIndex.end()) {
            for (auto it : iter->second) {
                auto priceIndex = it.first;
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
                auto priceIndex = it.first;
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

