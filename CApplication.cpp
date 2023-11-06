#include "CApplication.h"

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
}

void CApplication::Start() {
    m_TD = new PROTD::TDImpl(this);
    m_TD->Start();

    m_timer.expires_from_now(boost::posix_time::milliseconds(3000));
    m_timer.async_wait(boost::bind(&CApplication::OnTime, this, boost::asio::placeholders::error));
}

void CApplication::OnTime(const boost::system::error_code& error) {
    if (error) {
        if (error == boost::asio::error::operation_aborted) {
            return;
        }
    }

    for (auto& iter : m_watchSecurity) {
        if (!m_isTest || iter.second.ExchangeID == PROMD::TORA_TSTP_EXD_SSE) {
            if (m_shMD) {
                m_shMD->ShowOrderBook((char*)iter.first.c_str());
                m_shMD->ShowUnfindOrder((char*)iter.first.c_str());
            }
        } else if (iter.second.ExchangeID == PROMD::TORA_TSTP_EXD_SZSE) {
            if (m_szMD) {
                m_szMD->ShowOrderBook((char*)iter.first.c_str());
                m_szMD->ShowUnfindOrder((char*)iter.first.c_str());
            }
        }
    }
    m_timer.expires_from_now(boost::posix_time::milliseconds(6000));
    m_timer.async_wait(boost::bind(&CApplication::OnTime, this, boost::asio::placeholders::error));
}

/***************************************MD***************************************/
void CApplication::MDOnInited(PROMD::TTORATstpExchangeIDType exchangeID) {
    auto md = GetMDByExchangeID(exchangeID);
    if (!md) return;
    for (auto &iter: m_watchSecurity) { // change to marketSecurity
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
    if (!m_TD || !m_TD->IsInited()) return;
    //printf("MDPostPrice %s %lld %.2f|%.2f|%.2f %lld\n", postPrice.SecurityID, postPrice.AskVolume1, postPrice.AskPrice1, postPrice.TradePrice, postPrice.BidPrice1, postPrice.BidVolume1);

    auto iterSecurity = m_TD->m_marketSecurity.find(postPrice.SecurityID);
    if (iterSecurity == m_TD->m_marketSecurity.end()) {
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
void CApplication::TDOnInited() {
    if (m_isTest) {
        m_shMD = new PROMD::MDL2Impl(this, PROMD::TORA_TSTP_EXD_SSE);
        m_shMD->Start(m_isTest);
        m_szMD = new PROMD::MDL2Impl(this, PROMD::TORA_TSTP_EXD_SZSE);
        m_szMD->Start(m_isTest);
    }
    else {
        m_shMD = new PROMD::MDL2Impl(this, PROMD::TORA_TSTP_EXD_COMM);
        m_shMD->Start(m_isTest);
    }
}

PROMD::MDL2Impl *CApplication::GetMDByExchangeID(PROMD::TTORATstpExchangeIDType ExchangeID) {
    if (ExchangeID == PROMD::TORA_TSTP_EXD_SSE || ExchangeID == PROMD::TORA_TSTP_EXD_COMM) return m_shMD;
    return m_szMD;
}

bool CApplication::Init(std::string& watchSecurity) {
    trim(watchSecurity);
    std::vector<std::string> vtSecurity;
    Stringsplit(watchSecurity, ',', vtSecurity);
    for (auto iter : vtSecurity) {
        stSecurity_t security = {0};
        strcpy(security.SecurityID, iter.c_str());
        m_watchSecurity[security.SecurityID] = security;
    }

    //if (!LoadStrategy()) {
    //    printf("LoadStrategy Failed!!!\n");
    //    return false;
    //}
    return true;
}

bool CApplication::LoadStrategy() {
    m_db = SQLite3::Create(m_dbFile, SQLite3::READWRITE | SQLite3::CREATE);
    if (!m_db) return false;

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
