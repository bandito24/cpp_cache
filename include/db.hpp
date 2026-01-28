#pragma once
#include "http_client.hpp"
#include "sqlite3.h"
#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

struct RowData {
  std::string id;
  long exp_timestamp;
  std::string content;
};
enum ReturnStatus { CACHE_HIT, CACHE_MISS, CACHE_REFRESH };

struct ReturnData {
  ReturnStatus status;
  std::string content;
};

struct SqlDbDelelter {
  void operator()(sqlite3 *db) noexcept { sqlite3_close(db); }
};

struct iSqlClient {
  virtual ~iSqlClient() = default;
  virtual std::optional<RowData> query(const std::string &port_url) = 0;

  virtual void persistRowData(const RowData rowData,
                              const ReturnStatus status) = 0;
  virtual void create_database() = 0;
  virtual void check_rc(int rc) = 0;
  virtual int test() = 0;
};
class SqlClient : public iSqlClient {
public:
  ~SqlClient() override;
  SqlClient(const std::string &file_name);
  void persistRowData(const RowData rowData,
                      const ReturnStatus status) override;
  std::optional<RowData> query(const std::string &port_url) override;
  void create_database() override;
  int test() override;

  void check_rc(int rc) override;

private:
  std::unique_ptr<sqlite3, SqlDbDelelter> db;
};

struct iTimeGetter {
  virtual ~iTimeGetter() = default;
  virtual std::chrono::system_clock::time_point now() const = 0;
};
class TimeGetter : public iTimeGetter {
  std::chrono::system_clock::time_point now() const override;
};

class Db {
private:
  std::string file_name = "data.csv";
  std::filesystem::path full_path =
      std::filesystem::path(PROJECT_SOURCE_DIR) / file_name;

  std::chrono::seconds expiration_duration = std::chrono::minutes{45};

  std::string insert(const std::string &port_url);
  std::string quoteSql(const std::string &arg);
  std::string fetchData(const std::string &port_url);
  std::string update(const std::string &port_url);
  RowData generateRowData(const std::string &port_url);
  iSqlClient &sqlClient;
  iHttpClient &http_client;
  iTimeGetter &time_getter;

public:
  Db(int minute_cache_duration, iSqlClient &sqlClient, iHttpClient &http_client,
     iTimeGetter &time_getter);
  ReturnData read(const std::string &port_url);
  static std::string DEFAULT_FILENAME;
  int testIt();
};

enum class ROW_DEF { ID, PAGE_CONTENT, EXP_TIMESTAMP };

struct StatementDeleterFunction {
  void operator()(sqlite3_stmt *stmt) { sqlite3_finalize(stmt); }
};

using db_stmt = std::unique_ptr<sqlite3_stmt, StatementDeleterFunction>;

class Statement {
  std::string sql;
  sqlite3 *db;
  db_stmt stmt_;

public:
  Statement(std::string sql, sqlite3 *db);
  sqlite3_stmt *getStmt() const { return stmt_.get(); }
};
