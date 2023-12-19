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
        m_watchSecurity[security->SecurityID] = security;
    }
}

void CApplication::Start() {
    if (m_dataDir.length() > 0) {
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

    printf("----------------------------%d %s----------------------------\n", m_totalCount, GetTimeStr().c_str());
    for (auto& iter : m_watchSecurity) {
        ShowOrderBook((char*)iter.first.c_str());
    }

    m_timer.expires_from_now(boost::posix_time::milliseconds(60000));
    m_timer.async_wait(boost::bind(&CApplication::OnTime, this, boost::asio::placeholders::error));
}

/***************************************MD***************************************/
void CApplication::MDOnInitFinished(PROMD::TTORATstpExchangeIDType ExchangeID) {
    auto cnt = 0;
    //for (auto &iter: m_marketSecurity) {
    for (auto &iter: m_watchSecurity) {
        if (m_marketSecurity.find(iter.first) == m_marketSecurity.end()) continue;
        if (ExchangeID == iter.second->ExchangeID) {
            cnt++;
            PROMD::TTORATstpSecurityIDType SecurityID = {0};
            strncpy(SecurityID, iter.first.c_str(), sizeof(SecurityID));
            char *security_arr[1];
            security_arr[0] = SecurityID;
            if (iter.second->ExchangeID == PROMD::TORA_TSTP_EXD_SSE && m_isSHNewversion) {
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
    m_totalCount++;
    m_SecurityIDNotifyCount[OrderDetail.SecurityID]++;
    if (OrderDetail.OrderType == PROMD::TORA_TSTP_LOT_Market) return;
    if (OrderDetail.Side != PROMD::TORA_TSTP_LSD_Buy && OrderDetail.Side != PROMD::TORA_TSTP_LSD_Sell) return;

    if (OrderDetail.ExchangeID == PROMD::TORA_TSTP_EXD_SZSE) {
        if (OrderDetail.OrderType == PROMD::TORA_TSTP_LOT_HomeBest) {
            AddHomebestOrder(OrderDetail.SecurityID, OrderDetail.OrderNO, OrderDetail.Volume, OrderDetail.Side);
            return;
        }
        InsertOrder(OrderDetail.SecurityID, OrderDetail.OrderNO, OrderDetail.Price, OrderDetail.Volume, OrderDetail.Side);
    }
    else if (OrderDetail.ExchangeID == PROMD::TORA_TSTP_EXD_SSE) {
        if (OrderDetail.OrderStatus == PROMD::TORA_TSTP_LOS_Add) {
            InsertOrder(OrderDetail.SecurityID, OrderDetail.OrderNO, OrderDetail.Price, OrderDetail.Volume, OrderDetail.Side);
        } else if (OrderDetail.OrderStatus == PROMD::TORA_TSTP_LOS_Delete) {
            ModifyOrder(OrderDetail.SecurityID, 0, OrderDetail.OrderNO, OrderDetail.Side);
        }
    }
}

void CApplication::MDOnRtnTransaction(PROMD::CTORATstpLev2TransactionField &Transaction) {
    m_totalCount++;
    m_SecurityIDNotifyCount[Transaction.SecurityID]++;
    if (Transaction.ExchangeID == PROMD::TORA_TSTP_EXD_SZSE) {
        if (Transaction.ExecType == PROMD::TORA_TSTP_ECT_Fill) {
            {
                auto homeBestBuyOrder = GetHomebestOrder(Transaction.SecurityID, Transaction.BuyNo);
                if (homeBestBuyOrder) {
                    homeBestBuyOrder->Volume -= Transaction.TradeVolume;
                    if (homeBestBuyOrder->Volume > 0) {
                        InsertOrder(homeBestBuyOrder->SecurityID, homeBestBuyOrder->OrderNO, Transaction.TradePrice, homeBestBuyOrder->Volume, homeBestBuyOrder->Side);
                    }
                    DelHomebestOrder(Transaction.SecurityID, Transaction.BuyNo);
                } else {
                    ModifyOrder(Transaction.SecurityID, Transaction.TradeVolume, Transaction.BuyNo, PROMD::TORA_TSTP_LSD_Buy);
                }
            }
            {
                auto homeBestSellOrder = GetHomebestOrder(Transaction.SecurityID, Transaction.SellNo);
                if (homeBestSellOrder) {
                    homeBestSellOrder->Volume -= Transaction.TradeVolume;
                    if (homeBestSellOrder->Volume > 0) {
                        InsertOrder(homeBestSellOrder->SecurityID, homeBestSellOrder->OrderNO, Transaction.TradePrice, homeBestSellOrder->Volume, homeBestSellOrder->Side);
                    }
                    DelHomebestOrder(Transaction.SecurityID, Transaction.SellNo);
                } else {
                    ModifyOrder(Transaction.SecurityID, Transaction.TradeVolume, Transaction.SellNo, PROMD::TORA_TSTP_LSD_Sell);
                }
            }
        } else if (Transaction.ExecType == PROMD::TORA_TSTP_ECT_Cancel) {
            if (Transaction.BuyNo > 0) {
                ModifyOrder(Transaction.SecurityID, 0, Transaction.BuyNo, PROMD::TORA_TSTP_LSD_Buy);
            } else if (Transaction.SellNo > 0) {
                ModifyOrder(Transaction.SecurityID, 0, Transaction.SellNo, PROMD::TORA_TSTP_LSD_Sell);
            }
        }
    }
    else if (Transaction.ExchangeID == PROMD::TORA_TSTP_EXD_SSE) {
        ModifyOrder(Transaction.SecurityID, Transaction.TradeVolume, Transaction.BuyNo, PROMD::TORA_TSTP_LSD_Buy);
        ModifyOrder(Transaction.SecurityID, Transaction.TradeVolume, Transaction.SellNo, PROMD::TORA_TSTP_LSD_Sell);
    }
}

void CApplication::MDOnRtnNGTSTick(PROMD::CTORATstpLev2NGTSTickField &Tick) {
    m_totalCount++;
    m_SecurityIDNotifyCount[Tick.SecurityID]++;
    if (Tick.ExchangeID != PROMD::TORA_TSTP_EXD_SSE) return;
    if (Tick.Side != PROMD::TORA_TSTP_LSD_Buy && Tick.Side != PROMD::TORA_TSTP_LSD_Sell) return;

    if (Tick.TickType == PROMD::TORA_TSTP_LTT_Add) {
        InsertOrder(Tick.SecurityID, Tick.Side==PROMD::TORA_TSTP_LSD_Buy?Tick.BuyNo:Tick.SellNo, Tick.Price, Tick.Volume, Tick.Side);
    } else if (Tick.TickType == PROMD::TORA_TSTP_LTT_Delete) {
        ModifyOrder(Tick.SecurityID, 0, Tick.Side==PROMD::TORA_TSTP_LSD_Buy?Tick.BuyNo:Tick.SellNo, Tick.Side);
    } else if (Tick.TickType == PROMD::TORA_TSTP_LTT_Trade) {
        ModifyOrder(Tick.SecurityID, Tick.Volume, Tick.BuyNo, PROMD::TORA_TSTP_LSD_Buy);
        ModifyOrder(Tick.SecurityID, Tick.Volume, Tick.SellNo, PROMD::TORA_TSTP_LSD_Sell);
    }
}

void CApplication::InsertOrder(PROMD::TTORATstpSecurityIDType SecurityID, PROMD::TTORATstpLongSequenceType OrderNO, PROMD::TTORATstpPriceType Price, PROMD::TTORATstpLongVolumeType Volume, PROMD::TTORATstpLSideType Side) {
    int SecurityIDInt = atoi(SecurityID);
    AddOrderNoPrice(SecurityIDInt, OrderNO, Price);

    auto order = m_pool.Malloc<stOrder>(sizeof(stOrder));
    order->OrderNo = OrderNO;
    order->Volume = Volume;

    auto added = false;
    auto &mapOrder = Side == PROMD::TORA_TSTP_LSD_Buy ? m_orderBuyV : m_orderSellV;
    auto iter = mapOrder.find(SecurityID);
    if (iter == mapOrder.end()) {
        mapOrder[SecurityID] = std::vector<stPriceOrders>();
        iter = mapOrder.find(SecurityID);
    } else if (!iter->second.empty()) {
        auto size = (int) iter->second.size();
        for (auto i = 0; i < size; i++) {
            auto nextPrice = iter->second.at(i).Price;
            if (Side == PROMD::TORA_TSTP_LSD_Buy) {
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
            } else if (Side == PROMD::TORA_TSTP_LSD_Sell) {
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

void CApplication::ModifyOrder(PROMD::TTORATstpSecurityIDType SecurityID, PROMD::TTORATstpLongVolumeType Volume, PROMD::TTORATstpLongSequenceType OrderNo, PROMD::TTORATstpTradeBSFlagType Side) {
    int SecurityIDInt = atoi(SecurityID);
    auto Price = GetOrderNoPrice(SecurityIDInt, OrderNo);
    if (Price < 0.000001) {
        //printf("未找到订单号为:%lld 的合约:%s 数量:%lld\n", OrderNo, SecurityID, Volume);
        return;
    }

    auto &mapOrder = Side == PROMD::TORA_TSTP_LSD_Buy ? m_orderBuyV : m_orderSellV;
    auto iter = mapOrder.find(SecurityID);
    if (iter == mapOrder.end()) return;

    for (auto iterVecOrder = iter->second.begin(); iterVecOrder != iter->second.end();) {
        auto curPrice = iterVecOrder->Price;
        if ((Side == PROMD::TORA_TSTP_LSD_Buy && curPrice > Price + 0.000001) ||
            (Side == PROMD::TORA_TSTP_LSD_Sell && curPrice + 0.000001 < Price)) {
            if (Volume > 0) {
                for (auto iterOrder = iterVecOrder->Orders.begin(); iterOrder != iterVecOrder->Orders.end(); ++iterOrder) {
                    auto order = (*iterOrder);
                    DelOrderNoPrice(SecurityIDInt, order->OrderNo);
                    m_pool.Free<stOrder>(order, sizeof(stOrder));
                }
                iterVecOrder = iter->second.erase(iterVecOrder);
            } else {
                ++iterVecOrder;
            }
        } else if ((Side == PROMD::TORA_TSTP_LSD_Buy && curPrice > Price - 0.000001) ||
                   (Side == PROMD::TORA_TSTP_LSD_Sell && curPrice - 0.000001 < Price)) {
            for (auto iterOrder = iterVecOrder->Orders.begin(); iterOrder != iterVecOrder->Orders.end();) {
                auto order = (*iterOrder);
                if (order->OrderNo == OrderNo) {
                    if (Volume > 0) {
                        (*iterOrder)->Volume -= Volume;
                    }
                    if (Volume == 0 || (*iterOrder)->Volume <= 0) {
                        auto order = (*iterOrder);
                        DelOrderNoPrice(SecurityIDInt, order->OrderNo);
                        iterOrder = iterVecOrder->Orders.erase(iterOrder);
                        m_pool.Free<stOrder>(order, sizeof(stOrder));
                    } else {
                        ++iterOrder;
                    }
                } else {
                    if (order->OrderNo < OrderNo) {
                        if (Volume > 0) {
                            DelOrderNoPrice(SecurityIDInt, order->OrderNo);
                            m_pool.Free<stOrder>(order, sizeof(stOrder));
                            iterOrder = iterVecOrder->Orders.erase(iterOrder);
                            continue;
                        }
                    }
                    ++iterOrder;
                    //break; // 上海有序后直接break
                }
            }
            if (iterVecOrder->Orders.empty()) {
                iterVecOrder = iter->second.erase(iterVecOrder);
            } else {
                ++iterVecOrder;
            }
        } else {
            break;
        }
    }
}

double CApplication::GetOrderNoPrice(int SecurityIDInt, PROMD::TTORATstpLongSequenceType OrderNO) {
    auto Price = 0.0;
    auto iter = m_orderNoPrice.find(SecurityIDInt);
    if (iter != m_orderNoPrice.end()) {
        auto it = iter->second.find(OrderNO);
        if (it != iter->second.end()) {
            Price = it->second;
        }
    }
    return Price;
}

void CApplication::AddOrderNoPrice(int SecurityIDInt, PROMD::TTORATstpLongSequenceType OrderNO, PROMD::TTORATstpPriceType Price) {
    auto iter = m_orderNoPrice.find(SecurityIDInt);
    if (iter != m_orderNoPrice.end()) {
        auto it = iter->second.find(OrderNO);
        if (it != iter->second.end()) {
            printf("添加 %d 订单号 %lld 价格错误 已经存在该订单号价格 %.3f\n", SecurityIDInt, OrderNO, Price);
        }
    }
    m_orderNoPrice[SecurityIDInt][OrderNO] = Price;
}

void CApplication::DelOrderNoPrice(int SecurityIDInt, PROMD::TTORATstpLongSequenceType OrderNO) {
    auto iter = m_orderNoPrice.find(SecurityIDInt);
    if (iter != m_orderNoPrice.end()) {
        auto it = iter->second.find(OrderNO);
        if (it != iter->second.end()) {
            m_orderNoPrice[SecurityIDInt].erase(OrderNO);
        }
    }
}

void CApplication::AddHomebestOrder(PROMD::TTORATstpSecurityIDType SecurityID, PROMD::TTORATstpLongSequenceType OrderNO,
                                    PROMD::TTORATstpLongVolumeType Volume, PROMD::TTORATstpLSideType Side) {
    auto homeBestOrder = m_pool.Malloc<stHomebestOrder>(sizeof(stHomebestOrder));
    strcpy(homeBestOrder->SecurityID, SecurityID);
    homeBestOrder->OrderNO = OrderNO;
    homeBestOrder->Volume = Volume;
    homeBestOrder->Side = Side;

    auto iter = m_homeBaseOrder.find(SecurityID);
    if (iter != m_homeBaseOrder.end()) {
        if (auto it = iter->second.find(OrderNO) != iter->second.end()) {
            printf("添加 %s 最优价订单 %lld %c 已存在该订单号!!!\n", SecurityID, OrderNO, Side);
        }
    }
    m_homeBaseOrder[homeBestOrder->SecurityID][homeBestOrder->OrderNO] = homeBestOrder;
}

stHomebestOrder* CApplication::GetHomebestOrder(PROMD::TTORATstpSecurityIDType SecurityID, PROMD::TTORATstpLongSequenceType OrderNO) {
    auto iter = m_homeBaseOrder.find(SecurityID);
    if (iter == m_homeBaseOrder.end()) return nullptr;
    if (iter->second.find(OrderNO) == iter->second.end()) return nullptr;
    return iter->second[OrderNO];
}

void CApplication::DelHomebestOrder(PROMD::TTORATstpSecurityIDType SecurityID, PROMD::TTORATstpLongSequenceType OrderNO) {
    auto iter = m_homeBaseOrder.find(SecurityID);
    if (iter == m_homeBaseOrder.end()) return;
    if (iter->second.find(OrderNO) == iter->second.end()) return;
    auto ptr = iter->second[OrderNO];
    iter->second.erase(OrderNO);
    m_pool.Free<stHomebestOrder>(ptr, sizeof(stHomebestOrder));
}

void CApplication::ShowOrderBook(PROMD::TTORATstpSecurityIDType SecurityID) {
    if (m_orderBuyV.find(SecurityID) == m_orderBuyV.end() &&
        m_orderSellV.find(SecurityID) == m_orderSellV.end()) return;
    int securityIDCount = 0;
    if (m_SecurityIDNotifyCount.find(SecurityID) != m_SecurityIDNotifyCount.end()) {
        securityIDCount = m_SecurityIDNotifyCount[SecurityID];
    }
    std::stringstream stream;
    stream << "\n";
    stream << "-------------" << SecurityID << " " << securityIDCount << "-------------\n";

    char buffer[512] = {0};
    {
        std::vector<std::string> temp;
        auto iter = m_orderSellV.find(SecurityID);
        if (iter != m_orderSellV.end()) {
            int i = 1;
            for (auto iter1: iter->second) {
                memset(buffer, 0, sizeof(buffer));
                auto totalVolume = 0;
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
                auto totalVolume = 0;
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

/***************************************TD***************************************/
void CApplication::TDOnRspQrySecurity(PROTD::CTORATstpSecurityField &Security) {
    auto iter = m_marketSecurity.find(Security.SecurityID);
    if (iter == m_marketSecurity.end()) {
        auto security = m_pool.Malloc<stSecurity>(sizeof(stSecurity));
        if (security) {
            strcpy(security->SecurityID, Security.SecurityID);
            security->ExchangeID = Security.ExchangeID;
            security->UpperLimitPrice = Security.UpperLimitPrice;
            security->LowerLimitPrice = Security.LowerLimitPrice;
            m_marketSecurity[security->SecurityID] = security;
        }
    }
    if (m_watchSecurity.find(Security.SecurityID) != m_watchSecurity.end()) {
        m_watchSecurity[Security.SecurityID]->ExchangeID = Security.ExchangeID;
    }
}

void CApplication::TDOnInitFinished() {
    printf("CApplication::TDOnInitFinished\n");

    if (m_MD) return;
    PROMD::TTORATstpExchangeIDType ExchangeID = PROMD::TORA_TSTP_EXD_SZSE;
    if (m_isSHExchange) ExchangeID = PROMD::TORA_TSTP_EXD_SSE;
    m_MD = new PROMD::MDL2Impl(this, ExchangeID);
    m_MD->Start(m_isTest);
}