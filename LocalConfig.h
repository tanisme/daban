//
// Created by 139-202 on 2023/10/6.
//

#ifndef BALIBALI_LOCALCONFIG_H
#define BALIBALI_LOCALCONFIG_H

#include "Singleton.h"
#include "sqliteimpl.h"

#include <unordered_map>
#include <string>

typedef struct stSecurity_s {
    char SecurityID[31];
    char SecurityName[81];
    char ExchangeID;
} stSecurity_t;

typedef struct stStrategy_s {
    int idx;
    char SecurityID[31];
    char ExchangeID;
} stStrategy_t;

class LocalConfig : public Singleton<LocalConfig> {
public:
    bool Init();

    bool LoadSubSecurity();

    bool AddSubSecurity(char *SecurityID, char ExchangeID);

    bool DelSubSecurity(char *SecurityID);

    bool LoadStrategy();

    std::string GetDBFile() const { return m_dbFile; }

    void SetDBFile(std::string &dbFile) { m_dbFile = dbFile; }

    std::string GetSHMDAddr() const { return m_shMDAddr; }

    void SetSHMDAddr(std::string &addr) { m_shMDAddr = addr; }

    std::string GetSZMDAddr() const { return m_szMDAddr; }

    void SetSZMDAddr(std::string &addr) { m_szMDAddr = addr; }

    std::string GetTDAddr() const { return m_TDAddr; }

    void SetTDAddr(std::string &addr) { m_TDAddr = addr; }

    std::string GetTDAccount() const { return m_TDAccount; }

    void SetTDAccount(std::string &account) { m_TDAccount = account; }

    std::string GetTDPassword() const { return m_TDPassword; }

    void SetTDPassword(std::string &password) { m_TDPassword = password; }

public:
    std::unordered_map<std::string, stSecurity_t> m_mapSecurityID;

private:
    std::string m_dbFile;
    std::string m_shMDAddr = "tcp://210.14.72.17:16900";
    std::string m_szMDAddr = "tcp://210.14.72.17:6900";
    std::string m_TDAddr = "tcp://210.14.72.21:4400";
    std::string m_TDAccount = "00030557";
    std::string m_TDPassword = "17522830";
    SQLite3::ptr m_db = nullptr;
};

#endif //BALIBALI_LOCALCONFIG_H
