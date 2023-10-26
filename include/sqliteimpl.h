//
// Created by 139-202 on 2023/10/8.
//

#ifndef BALIBALI_SQLITE_H
#define BALIBALI_SQLITE_H

#include "db.h"
#include "sqlite3.h"

#include <memory>
#include <string>

class SQLite3 : public ISQLUpdate, public ISQLQuery
    , public std::enable_shared_from_this<SQLite3> {
public:
    enum Flags {
        READONLY = SQLITE_OPEN_READONLY,
        READWRITE = SQLITE_OPEN_READWRITE,
        CREATE = SQLITE_OPEN_CREATE
    };

    typedef std::shared_ptr<SQLite3> ptr;
    static SQLite3::ptr Create(sqlite3* db);
    static SQLite3::ptr Create(const std::string& dbname, int flags = READONLY | CREATE);
    ~SQLite3();

    int getErrorCode() const;
    std::string getErrorMsg() const;
    int execute(const char* format, ...) override;
    int execute(const std::string& sql) override;
    int64_t getLastInsertId() override;
    ISQLData::ptr query(const char* format, ...) override;
    ISQLData::ptr query(const std::string& sql) override;

    template<typename... Args>
    int execStmt(const char* stmt, Args&&... args);

    template<class... Args>
    ISQLData::ptr queryStmt(const char* stmt, const Args&... args);

    int close();
    sqlite3* getDB() const { return m_db; }

private:
    SQLite3(sqlite3* db);

private:
    sqlite3* m_db;
};

class SQLite3Stmt;
class SQLite3Data : public ISQLData {
public:
    typedef std::shared_ptr<SQLite3Data> ptr;

    SQLite3Data(std::shared_ptr<SQLite3Stmt> stmt);

    int getDataCount() override;
    int getColumnCount() override;
    int getColumnBytes(int idx) override;
    int getColumnType(int idx) override;
    std::string getColumnName(int idx) override;

    int getInt(int idx) override;
    double getDouble(int idx) override;
    int64_t getInt64(int idx) override;
    const char* getText(int idx) override;
    std::string getTextString(int idx) override;
    std::string getBlob(int idx) override;
    bool next() override;

private:
    std::shared_ptr<SQLite3Stmt> m_stmt;
};

class SQLite3Stmt : public std::enable_shared_from_this<SQLite3Stmt> {
    friend class SQLite3Data;
public:
    typedef std::shared_ptr<SQLite3Stmt> ptr;
    enum Type {
        COPY = 1,
        REF = 2
    };

    static SQLite3Stmt::ptr Create(SQLite3::ptr db, const char* stmt);
    virtual ~SQLite3Stmt();

    int prepare(const char* stmt);
    int finish();

    int bind(int idx, int32_t value);
    int bind(int idx, double value);
    int bind(int idx, int64_t value);
    int bind(int idx, const char* value, Type type = COPY);
    int bind(int idx, const void* value, int len, Type type = COPY);
    int bind(int idx, const std::string& value, Type type = COPY);
    int bind(int idx);

    int bind(const char* name, int32_t value);
    int bind(const char* name, double value);
    int bind(const char* name, int64_t value);
    int bind(const char* name, const char* value, Type type = COPY);
    int bind(const char* name, const void* value, int len, Type type = COPY);
    int bind(const char* name, const std::string& value, Type type = COPY);
    int bind(const char* name);

    int step();
    int reset();

    SQLite3Data::ptr query();
    int execute();

protected:
    SQLite3Stmt(SQLite3::ptr db);

private:
    SQLite3::ptr m_db;
    sqlite3_stmt* m_stmt;
};

#endif //BALIBALI_SQLITE_H
