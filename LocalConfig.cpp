//
// Created by 139-202 on 2023/10/6.
//

#include "LocalConfig.h"
#include <string.h>

bool LocalConfig::Init() {
    m_db = SQLite3::Create(m_dbFile, SQLite3::READWRITE | SQLite3::CREATE);
    if (m_db == nullptr) {
        return false;
    }

    if (!LoadSubSecurity()) {
        printf("加载待订阅合约失败\n");
        return false;
    }
    if (!LoadStrategy()) {
        printf("加载策略失败\n");
        return false;
    }
    return true;
}

bool LocalConfig::LoadSubSecurity() {
    const char *create_tab_sql = "CREATE TABLE IF NOT EXISTS subsecurity("
                                 "security CHAR(32) PRIMARY KEY,"
                                 "securityname CHAR(32),"
                                 "exchange CHAR NOT NULL,"
                                 "status INTEGER NOT NULL DEFAULT 0);";
    m_db->execute(create_tab_sql);

    const char *select_sql = "SELECT security, securityname, exchange FROM subsecurity where status=0;";
    SQLite3Stmt::ptr query = SQLite3Stmt::Create(m_db, select_sql);
    if (!query) {
        return false;
    }

    auto ds = query->query();
    stSecurity_t security;
    while (ds->next()) {
        memset(&security, 0, sizeof(security));
        strcpy(security.SecurityID, ds->getText(0));
        strcpy(security.SecurityName, ds->getText(1));
        security.ExchangeID = ds->getText(2)[0];
        m_mapSecurityID[security.SecurityID] = security;
    }

    return true;
}

bool LocalConfig::AddSubSecurity(char *SecurityID, char ExchangeID) {
    auto ret = m_db->execute("INSERT INTO subsecurity(security,exchange) VALUES(%s,%c);", SecurityID, ExchangeID);
    if (ret != SQLITE_OK) {
        return false;
    }
    stSecurity_t security = {0};
    strcpy(security.SecurityID, SecurityID);
    security.ExchangeID = ExchangeID;
    m_mapSecurityID[security.SecurityID] = security;
    return true;
}

bool LocalConfig::DelSubSecurity(char *SecurityID) {
    auto iter = m_mapSecurityID.find(SecurityID);
    if (iter == m_mapSecurityID.end()) return true;

    auto ret = m_db->execute("DELETE FROM subsecurity WHERE security=%s;", SecurityID);
    if (ret != SQLITE_OK) {
        return false;
    }
    m_mapSecurityID.erase(SecurityID);
    return true;
}

bool LocalConfig::LoadStrategy() {
    int i = 0;
    const char *create_tab_sql = "CREATE TABLE IF NOT EXISTS subsecurity("
                                 "idx INTEGER PRIMARY KEY AUTOINCREMENT,"
                                 "security CHAR(32) NOT NULL,"
                                 "exchange CHAR NOT NULL);";
    return true;
}