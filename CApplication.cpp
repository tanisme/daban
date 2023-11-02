#include "CApplication.h"
#include <iostream>
#include <boost/bind/bind.hpp>

CApplication::CApplication(boost::asio::io_context& ioc)
        : m_ioc(ioc)
        , m_timer(m_ioc, boost::posix_time::milliseconds(1000)) {
}

CApplication::~CApplication() {
    if (m_shMD != nullptr) {
        m_shMD->GetApi()->Join();
        m_shMD->GetApi()->Release();
        delete m_shMD;
    }
    if (m_szMD != nullptr) {
        m_szMD->GetApi()->Join();
        m_szMD->GetApi()->Release();
        delete m_szMD;
    }

    if (m_TD != nullptr) {
        m_TD->GetApi()->Join();
        m_TD->GetApi()->Release();
        delete m_TD;
    }

    for (auto iter = m_securityIDs.begin(); iter != m_securityIDs.end(); ++iter) {
        auto ptr = iter->second;
        m_securityIDs.erase(iter);
        free(ptr);
    }
}

void CApplication::Start() {
    if (m_isTest) {
        m_shMD = new PROMD::MDL2Impl(this, PROMD::TORA_TSTP_EXD_SSE);
        m_shMD->Start(m_useTcp);
        m_szMD = new PROMD::MDL2Impl(this, PROMD::TORA_TSTP_EXD_SZSE);
        m_szMD->Start(m_useTcp);
    }
    else {
        m_shMD = new PROMD::MDL2Impl(this, PROMD::TORA_TSTP_EXD_COMM);
        m_shMD->Start(m_useTcp);
    }

    //m_TD = new PROTD::TDImpl(this);
    //m_TD->Start();

    m_timer.expires_from_now(boost::posix_time::milliseconds(3000));
    m_timer.async_wait(boost::bind(&CApplication::OnTime, this, boost::asio::placeholders::error));
}

void CApplication::OnTime(const boost::system::error_code& error)
{
    if (error) {
        if (error == boost::asio::error::operation_aborted) {
            return;
        }
    }

    for (auto& iter : m_subSecurityIDs) {
        if (iter.second.Status == 1) continue;
        if (!m_isTest || iter.second.ExchangeID == PROMD::TORA_TSTP_EXD_SSE) {
            //if (m_shMD) m_shMD->ShowFixOrderBook((char*)iter.first.c_str());
            if (m_shMD) m_shMD->GenOrderBook((char*)iter.first.c_str());
        } else if (iter.second.ExchangeID == PROMD::TORA_TSTP_EXD_SZSE) {
            //if (m_szMD) m_szMD->ShowFixOrderBook((char*)iter.first.c_str());
            if (m_szMD) m_szMD->GenOrderBook((char*)iter.first.c_str());
        }
    }
    m_timer.expires_from_now(boost::posix_time::milliseconds(60000));
    m_timer.async_wait(boost::bind(&CApplication::OnTime, this, boost::asio::placeholders::error));
}

void CApplication::OnHttpHandle(int ServiceNo) {
    printf("Onfilehandle\n");
}

/***************************************MD***************************************/
void CApplication::MDOnRspUserLogin(PROMD::TTORATstpExchangeIDType exchangeID) {
    auto md = GetMDByExchangeID(exchangeID);
    if (!md) return;
    for (auto &iter: m_subSecurityIDs) {
        if (iter.second.Status > 0) continue;
        if (exchangeID == iter.second.ExchangeID || exchangeID == PROMD::TORA_TSTP_EXD_COMM) {
            PROMD::TTORATstpSecurityIDType Security = {0};
            strncpy(Security, iter.first.c_str(), sizeof(Security));
            if (iter.second.ExchangeID == PROMD::TORA_TSTP_EXD_SSE) {
                if (m_shMDNewVersion > 0) {
                    md->ReqMarketData(Security, iter.second.ExchangeID, 4);
                } else {
                    md->ReqMarketData(Security, iter.second.ExchangeID, 2);
                    md->ReqMarketData(Security, iter.second.ExchangeID, 3);
                }
            } else if (iter.second.ExchangeID == PROMD::TORA_TSTP_EXD_SZSE) {
                md->ReqMarketData(Security, iter.second.ExchangeID, 2);
                md->ReqMarketData(Security, iter.second.ExchangeID, 3);
            }
        }
    }
}

void CApplication::MDPostPrice(stPostPrice& postPrice) {
    if (true) return;
    if (!m_TD->IsLogined()) return;
    //printf("MDPostPrice %s %lld %.2f|%.2f|%.2f %lld\n", postPrice.SecurityID, postPrice.AskVolume1, postPrice.AskPrice1, postPrice.TradePrice, postPrice.BidPrice1, postPrice.BidVolume1);

    auto iterSecurity = m_securityIDs.find(postPrice.SecurityID);
    if (iterSecurity == m_securityIDs.end()) {
        //printf("MDPostPrice Security not found!!!\n");
        return;
    }
    auto iterStrategy = m_strategys.find(postPrice.SecurityID);
    if (iterStrategy == m_strategys.end()) {
        return;
    }

    auto UpperLimitPrice = iterSecurity->second->UpperLimitPrice;//涨停价
    /* type=1.扫涨停：只使用逐笔成交的成交价判断距离涨停价有多少差距，如果满足则下单。
      type=2.触涨停：需要合成实时的订单簿，根据每次计算出的订单簿取卖一价是否涨停和卖一价档位的数量计算卖一档未成交的金额做比较，小于x万元则下单.
      type=3.排涨停：需要合成实时的订单簿，排涨停没有卖档，根据每次计算出的订单簿取买一档的数量计算买一档未成交的金额，如果大于x万元，则下单。 */
    for (auto iter = iterStrategy->second.begin(); iter != iterStrategy->second.end(); ++iter) {
        if (iter->status == 1) continue;
        auto rt = -1;
        if (iter->type == 1) { // p1-距涨停价x p2-买入的金额
            auto price = (postPrice.TradePrice + 0.01) * (1 + iter->params.p1);
            if (price >= UpperLimitPrice + 0.000001) {
                auto volume = (int)(iter->params.p2 / UpperLimitPrice);
                rt = m_TD->OrderInsert(postPrice.SecurityID, iterSecurity->second->ExchangeID,
                                       PROMD::TORA_TSTP_LSD_Buy, volume, UpperLimitPrice);
            }
        } else if (iter->type == 2) { // p1-卖1金额小于x p2-买入的金额
            if (postPrice.AskPrice1 >= UpperLimitPrice) {
                auto amount = postPrice.AskPrice1 * postPrice.AskVolume1;
                if (amount < iter->params.p1) {
                    auto volume = (int)iter->params.p2 / UpperLimitPrice;
                    rt = m_TD->OrderInsert(postPrice.SecurityID, iterSecurity->second->ExchangeID,
                                           PROMD::TORA_TSTP_LSD_Buy, volume, UpperLimitPrice);
                }
            }
        } else if (iter->type == 3) { // p1-买1金额大于x p2-买入的金额
            if (postPrice.AskPrice1 <= 0.00001 && postPrice.AskVolume1 == 0) {
                auto amount = postPrice.BidPrice1 * postPrice.BidVolume1;
                if (amount > iter->params.p1) {
                    auto volume = (int)iter->params.p2 / UpperLimitPrice;
                    rt = m_TD->OrderInsert(postPrice.SecurityID, iterSecurity->second->ExchangeID,
                                           PROMD::TORA_TSTP_LSD_Buy, volume, UpperLimitPrice);
                }
            }
        }
        if (rt == 0) {
            iter->status = 1;
            // TODO更新数据库
        }
    }
}

/***************************************TD***************************************/
void CApplication::TDOnRspUserLogin(PROTD::CTORATstpRspUserLoginField& RspUserLoginField) {
    printf("CApplication::TDOnRspUserLogin Success!!!\n");
}

void CApplication::TDOnRspQrySecurity(PROTD::CTORATstpSecurityField& Security) {
    //printf("SecurityID:%s,SecurityName:%s,UpperLimitPrice:%lf,LowerLimitPrice:%lf\n", Security.SecurityID, Security.SecurityName, Security.UpperLimitPrice, Security.LowerLimitPrice);
    if (m_securityIDs.find(Security.SecurityID) != m_securityIDs.end()) {
        auto security = m_securityIDs[Security.SecurityID];
        memcpy(security, &Security, sizeof(PROTD::CTORATstpSecurityField));
    } else {
        auto security = (PROTD::CTORATstpSecurityField *) malloc(sizeof(PROTD::CTORATstpSecurityField));
        if (security) {
            memcpy(security, &Security, sizeof(PROTD::CTORATstpSecurityField));
            m_securityIDs[security->SecurityID] = security;
        }
    }
}

void CApplication::TDOnRspQryOrder(PROTD::CTORATstpOrderField& Order) {
    printf("OrderLocalID:%s,SecurityID:%s,Direction:%c,VolumeTotalOriginal:%d,LimitPrice:%lf,VolumeTraded:%d,VolumeCanceled:%d,OrderSysID:%s,FrontID:%d,SessionID:%d,OrderRef:%d,OrderStatus:%c,StatusMsg:%s,InsertTime:%s\n",
           Order.OrderLocalID, Order.SecurityID, Order.Direction, Order.VolumeTotalOriginal, Order.LimitPrice,
           Order.VolumeTraded, Order.VolumeCanceled, Order.OrderSysID, Order.FrontID, Order.SessionID,
           Order.OrderRef, Order.OrderStatus, Order.StatusMsg, Order.InsertTime);
}

void CApplication::TDOnRspQryTrade(PROTD::CTORATstpTradeField& Trade) {
    printf("OrderLocalID:%s,SecurityID:%s,Direction:%c,Volume:%d,Price:%lf,OrderSysID:%s,OrderRef:%d,TradeTime:%s\n",
           Trade.OrderLocalID, Trade.SecurityID, Trade.Direction, Trade.Volume, Trade.Price,
           Trade.OrderSysID, Trade.OrderRef, Trade.TradeTime);
}

void CApplication::TDOnRspQryPosition(PROTD::CTORATstpPositionField& Position) {
    printf("SecurityID:%s,CurrentPosition:%d,OpenPosCost:%lf,TotalPosCost:%lf,PrePosition:%d,HistoryPosFrozen:%d\n",
           Position.SecurityID, Position.CurrentPosition, Position.OpenPosCost, Position.TotalPosCost,
           Position.PrePosition, Position.HistoryPosFrozen);
    //auto pos = (PROTD::CTORATstpPositionField *) malloc(sizeof(PROTD::CTORATstpPositionField));
    //if (pos) {
    //    memcpy(pos, pPosition, sizeof(*pos));

    //}
}

void CApplication::TDOnRspQryTradingAccount(PROTD::CTORATstpTradingAccountField& TradingAccount) {
    printf("AccountID:%s,UsefulMoney:%lf,FrozenCash:%lf,FrozenCommission:%lf\n",
           TradingAccount.AccountID, TradingAccount.UsefulMoney, TradingAccount.FrozenCash, TradingAccount.FrozenCommission);

    //auto rt = m_TD->OrderInsert("688179", PROMD::TORA_TSTP_LSD_Buy, 100, 18.95);
}

void CApplication::TDOnRspOrderInsert(PROTD::CTORATstpInputOrderField& pInputOrderField) {

}

void CApplication::TDOnErrRtnOrderInsert(PROTD::CTORATstpInputOrderField& pInputOrderField) {

}

void CApplication::TDOnRtnOrder(PROTD::CTORATstpOrderField& pOrder) {

}

void CApplication::TDOnRtnTrade(PROTD::CTORATstpTradeField& pTrade) {

}

void CApplication::TDOnRspOrderAction(PROTD::CTORATstpInputOrderActionField& pInputOrderActionField) {

}

void CApplication::TDOnErrRtnOrderAction(PROTD::CTORATstpInputOrderActionField& pInputOrderActionField) {

}

PROMD::MDL2Impl *CApplication::GetMDByExchangeID(PROMD::TTORATstpExchangeIDType ExchangeID) {
    if (ExchangeID == PROMD::TORA_TSTP_EXD_SSE || ExchangeID == PROMD::TORA_TSTP_EXD_COMM) return m_shMD;
    return m_szMD;
}

bool CApplication::Init() {
    m_db = SQLite3::Create(m_dbFile, SQLite3::READWRITE | SQLite3::CREATE);
    if (!m_db) return false;

    if (!LoadSubSecurityIDs()) {
        printf("LoadSubSecurityIDs Failed!!!\n");
        return false;
    }
    if (!LoadStrategy()) {
        printf("LoadStrategy Failed!!!\n");
        return false;
    }
    return true;
}

bool CApplication::LoadSubSecurityIDs() {
    const char *create_tab_sql = "CREATE TABLE IF NOT EXISTS subsecurity("
                                 "security CHAR(32) PRIMARY KEY,"
                                 "securityname CHAR(32),"
                                 "exchange CHAR NOT NULL,"
                                 "status INTEGER NOT NULL DEFAULT 0);";
    m_db->execute(create_tab_sql);

    const char *select_sql = "SELECT security,securityname,exchange,status FROM subsecurity;";
    SQLite3Stmt::ptr query = SQLite3Stmt::Create(m_db, select_sql);
    if (!query) return false;

    auto ds = query->query();
    stSubSecurity security;
    while (ds->next()) {
        memset(&security, 0, sizeof(security));
        strcpy(security.SecurityID, ds->getText(0));
        strcpy(security.SecurityName, ds->getText(1));
        security.ExchangeID = ds->getText(2)[0];
        security.Status = ds->getInt(3);
        m_subSecurityIDs[security.SecurityID] = security;
    }
    return true;
}

bool CApplication::AddSubSecurity(char *SecurityID, char ExchangeID) {
    //auto ret = m_db->execute("INSERT INTO subsecurity(security,exchange) VALUES(%s,%c);", SecurityID, ExchangeID);
    //if (ret != SQLITE_OK) {
    //    return false;
    //}
    //stSecurity_t security = {0};
    //strcpy(security.SecurityID, SecurityID);
    //security.ExchangeID = ExchangeID;
    //m_mapSecurityID[security.SecurityID] = security;
    return true;
}

bool CApplication::DelSubSecurity(char *SecurityID) {
    //auto iter = m_mapSecurityID.find(SecurityID);
    //if (iter == m_mapSecurityID.end()) return true;

    //auto ret = m_db->execute("DELETE FROM subsecurity WHERE security=%s;", SecurityID);
    //if (ret != SQLITE_OK) {
    //    return false;
    //}
    //m_mapSecurityID.erase(SecurityID);
    return true;
}

bool CApplication::LoadStrategy() {
    int i = 0;
    const char *create_tab_sql = "CREATE TABLE IF NOT EXISTS strategy("
                                 "idx INTEGER PRIMARY KEY AUTOINCREMENT,"
                                 "security CHAR(32) NOT NULL,"
                                 "type INTEGER NOT NULL DEFAULT 0,"
                                 "status INTEGER NOT NULL DEFAULT 0,"
                                 "param1 REAL,param2 REAL,param3 REAL,param4 REAL,param5 REAL"
                                 ");";

    m_db->execute(create_tab_sql);

    const char *select_sql = "SELECT idx,security,type,status,param1,param2,param3,param4,param5 FROM strategy;";
    SQLite3Stmt::ptr query = SQLite3Stmt::Create(m_db, select_sql);
    if (!query) return false;

    auto ds = query->query();
    stStrategy strategy;
    while (ds->next()) {
        memset(&strategy, 0, sizeof(strategy));
        strategy.idx = ds->getInt(0);
        strcpy(strategy.SecurityID, ds->getText(1));
        strategy.type = ds->getInt(2);
        strategy.status = ds->getInt(3);
        strategy.params.p1 = ds->getDouble(4);
        strategy.params.p2 = ds->getDouble(5);
        strategy.params.p3 = ds->getDouble(6);
        strategy.params.p4 = ds->getDouble(7);
        strategy.params.p5 = ds->getDouble(8);

        if (m_strategys.find(strategy.SecurityID) == m_strategys.end()) {
            m_strategys[strategy.SecurityID]  = std::vector<stStrategy>();
        }
        m_strategys[strategy.SecurityID].emplace_back(strategy);
    }
    return true;
}

bool CApplication::AddStrategy(stStrategy& strategy) {
    SQLite3Stmt::ptr stmt = SQLite3Stmt::Create(m_db,"insert into strategy(security,type,param1,param2,param3,param4,param5) values(?,?,?,?,?,?,?)");
    if(!stmt) {
        return false;
    }

    stmt->bind(1, strategy.SecurityID);
    stmt->bind(2, strategy.type);
    stmt->bind(3, strategy.params.p1);
    stmt->bind(4, strategy.params.p2);
    stmt->bind(5, strategy.params.p3);
    stmt->bind(6, strategy.params.p4);
    stmt->bind(7, strategy.params.p5);

    if(stmt->execute() != SQLITE_OK) {
        return false;
    }
    auto idx = m_db->getLastInsertId();
    strategy.idx = (int)idx;
    m_strategys[strategy.SecurityID].emplace_back(strategy);
    return true;
}
