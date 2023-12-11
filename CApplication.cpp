#include "CApplication.h"

CApplication::CApplication(boost::asio::io_context& ioc)
        : m_ioc(ioc)
        , m_timer(m_ioc, boost::posix_time::milliseconds(1000)) {
}

CApplication::~CApplication() {
    if (m_shMD) {
        m_shMD->GetApi()->Join();
        m_shMD->GetApi()->Release();
        delete m_shMD;
    }

    if (m_szMD) {
        m_szMD->GetApi()->Join();
        m_szMD->GetApi()->Release();
        delete m_szMD;
    }

    if (m_TD) {
        m_TD->GetApi()->Join();
        m_TD->GetApi()->Release();
        delete m_TD;
    }
}

bool CApplication::Init(std::string& watchSecurity, std::string& currentExchangeID) {
    trim(watchSecurity);
    std::vector<std::string> vtSecurity;
    Stringsplit(watchSecurity, ',', vtSecurity);
    for (auto& iter : vtSecurity) {
        auto security = m_pool.Malloc<stSecurity>(sizeof(stSecurity));
        strcpy(security->SecurityID, iter.c_str());
        m_watchSecurity[security->SecurityID] = security;
    }

    trim(currentExchangeID);
    std::vector<std::string> vtExchangeID;
    Stringsplit(currentExchangeID, ',', vtExchangeID);
    for (auto& iter : vtExchangeID) m_supportExchangeID[iter.c_str()[0]] = true;
    if (m_supportExchangeID.empty()) {
        printf("currentExchangeID config file not setted!!!\n");
        return false;
    }

    //if (!LoadStrategy()) {
    //    printf("LoadStrategy Failed!!!\n");
    //    return false;
    //}
    return true;
}

void CApplication::Start() {
    if (!m_TD) {
        m_TD = new PROTD::TDImpl(this);
        m_TD->Start(m_isTest);
    }

    m_timer.expires_from_now(boost::posix_time::milliseconds(5000));
    m_timer.async_wait(boost::bind(&CApplication::OnTime, this, boost::asio::placeholders::error));
}

void CApplication::OnTime(const boost::system::error_code& error) {
    if (error) {
        if (error == boost::asio::error::operation_aborted) {
            return;
        }
    }

    auto timestr = GetTimeStr();
    if ((strcmp(timestr.c_str(), "11:20:00") >= 0 && strcmp(timestr.c_str(), "11:40:00") <= 0) ||
        (strcmp(timestr.c_str(), "14:50:00") >= 0 && strcmp(timestr.c_str(), "15:10:00") <= 0)) {
        for (auto& iter : m_watchSecurity) {
            if (m_shMD && iter.second->ExchangeID == PROMD::TORA_TSTP_EXD_SSE) {
                m_shMD->ShowOrderBookV((char*)iter.first.c_str());
            }
            if (m_szMD && iter.second->ExchangeID == PROMD::TORA_TSTP_EXD_SZSE) {
                m_szMD->ShowOrderBookV((char*)iter.first.c_str());
            }
        }
    }

    if (m_shMD) m_shMD->ShowHandleSpeed();
    if (m_szMD) m_szMD->ShowHandleSpeed();

    m_timer.expires_from_now(boost::posix_time::milliseconds(6000));
    m_timer.async_wait(boost::bind(&CApplication::OnTime, this, boost::asio::placeholders::error));
}

/***************************************MD***************************************/
void CApplication::MDOnInited(PROMD::TTORATstpExchangeIDType exchangeID) {
    auto cnt = 0;
    std::unordered_map<std::string, stSecurity*>& sub = m_TD->m_marketSecurity;
    for (auto &iter: sub) {
        if (exchangeID == iter.second->ExchangeID &&
            (iter.second->SecurityType == PROTD::TORA_TSTP_STP_SHAShares ||
             //iter.second->SecurityType == PROTD::TORA_TSTP_STP_SHKC ||
             //iter.second->SecurityType == PROTD::TORA_TSTP_STP_SZGEM ||
             iter.second->SecurityType == PROTD::TORA_TSTP_STP_SZMainAShares)) {
            cnt++;
            PROMD::TTORATstpSecurityIDType Security = {0};
            strncpy(Security, iter.first.c_str(), sizeof(Security));
            if (iter.second->ExchangeID == PROMD::TORA_TSTP_EXD_SSE) {
                if (m_isSHNewversion) {
                    m_shMD->ReqMarketData(Security, iter.second->ExchangeID, 3);
                } else {
                    m_shMD->ReqMarketData(Security, iter.second->ExchangeID, 1);
                    m_shMD->ReqMarketData(Security, iter.second->ExchangeID, 2);
                }
            } else if (iter.second->ExchangeID == PROMD::TORA_TSTP_EXD_SZSE) {
                m_szMD->ReqMarketData(Security, iter.second->ExchangeID, 1);
                m_szMD->ReqMarketData(Security, iter.second->ExchangeID, 2);
            }
        }
    }
    printf("CApplication::MDOnInited %s subcount:%d total:%d\n",
           PROMD::MDL2Impl::GetExchangeName(exchangeID), cnt, (int)m_TD->m_marketSecurity.size());
}

void CApplication::MDPostPrice(stPostPrice& postPrice) {
    if (!m_TD || !m_TD->IsInited()) return;
    printf("MDPostPrice %s %lld %.2f|%.2f|%.2f %lld\n", postPrice.SecurityID, postPrice.AskVolume1, postPrice.AskPrice1, postPrice.TradePrice, postPrice.BidPrice1, postPrice.BidVolume1);

    auto iterSecurity = m_TD->m_marketSecurity.find(postPrice.SecurityID);
    if (iterSecurity == m_TD->m_marketSecurity.end()) return;
    auto iterStrategy = m_strategys.find(postPrice.SecurityID);
    if (iterStrategy == m_strategys.end()) return;

    auto UpperLimitPrice = iterSecurity->second->UpperLimitPrice;
    for (auto iter = iterStrategy->second.begin(); iter != iterStrategy->second.end(); ++iter) {
        if ((*iter)->status == 1) continue;
        auto rt = -1;
        if ((*iter)->type == 1) {
            auto price = (postPrice.TradePrice + 0.01) * (1 + (*iter)->params.p1);
            if (price >= UpperLimitPrice + 0.000001) {
                auto volume = (int)((*iter)->params.p2 / UpperLimitPrice);
                rt = m_TD->OrderInsert(postPrice.SecurityID, iterSecurity->second->ExchangeID, PROMD::TORA_TSTP_LSD_Buy, volume, UpperLimitPrice);
            }
        } else if ((*iter)->type == 2) {
            if (postPrice.AskPrice1 >= UpperLimitPrice) {
                auto amount = postPrice.AskPrice1 * postPrice.AskVolume1;
                if (amount < (*iter)->params.p1) {
                    auto volume = (int)(*iter)->params.p2 / UpperLimitPrice;
                    rt = m_TD->OrderInsert(postPrice.SecurityID, iterSecurity->second->ExchangeID, PROMD::TORA_TSTP_LSD_Buy, volume, UpperLimitPrice);
                }
            }
        } else if ((*iter)->type == 3) {
            if (postPrice.AskPrice1 <= 0.00001 && postPrice.AskVolume1 == 0) {
                auto amount = postPrice.BidPrice1 * postPrice.BidVolume1;
                if (amount > (*iter)->params.p1) {
                    auto volume = (int)(*iter)->params.p2 / UpperLimitPrice;
                    rt = m_TD->OrderInsert(postPrice.SecurityID, iterSecurity->second->ExchangeID, PROMD::TORA_TSTP_LSD_Buy, volume, UpperLimitPrice);
                }
            }
        }
        if (rt == 0) {
            (*iter)->status = 1;
            ModStrategy(*iter);
        }
    }
}

/***************************************TD***************************************/
void CApplication::TDOnInited() {
    auto iterSH = m_supportExchangeID.find(PROMD::TORA_TSTP_EXD_SSE);
    if (iterSH != m_supportExchangeID.end() && !m_shMD) {
        m_shMD = new PROMD::MDL2Impl(this, PROMD::TORA_TSTP_EXD_SSE);
        m_shMD->Start(m_isTest);
    }
    auto iterSZ = m_supportExchangeID.find(PROMD::TORA_TSTP_EXD_SZSE);
    if (iterSZ != m_supportExchangeID.end() && !m_szMD) {
        m_szMD = new PROMD::MDL2Impl(this, PROMD::TORA_TSTP_EXD_SZSE);
        m_szMD->Start(m_isTest);
    }
}

/*************************************Http****************************************/
bool CApplication::LoadStrategy() {
    const char *create_tab_sql = "CREATE TABLE IF NOT EXISTS strategy("
                                 "idx INTEGER PRIMARY KEY AUTOINCREMENT,"
                                 "security CHAR(32) NOT NULL,"
                                 "type INTEGER NOT NULL DEFAULT 0,"
                                 "status INTEGER NOT NULL DEFAULT 0,"
                                 "param1 REAL,param2 REAL,param3 REAL,param4 REAL,param5 REAL"
                                 ");";
    return true;
}

bool CApplication::AddStrategy(stStrategy& strategy) {
    return true;
}

bool CApplication::ModStrategy(stStrategy *strategy) {
    return true;
}