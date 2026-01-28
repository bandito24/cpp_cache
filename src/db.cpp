#include "http_client.hpp"
#include <chrono>
#include <cstddef>
#include <db.hpp>
#include <iostream>
#include <memory>
#include <optional>
#include <sqlite3.h>
#include <stdexcept>

using namespace std;

std::string Db::DEFAULT_FILENAME = "data.db";

SqlClient::~SqlClient() = default;

SqlClient::SqlClient(const std::string &file_name) {
  sqlite3 *dbConn = nullptr;

  int rc;
  rc = sqlite3_open(file_name.c_str(), &dbConn);
  if (rc) {
    throw std::runtime_error(sqlite3_errmsg(dbConn));
  }
  this->db.reset(dbConn);
}
int SqlClient::test() { return 1; }

void SqlClient::create_database() {
  char *errMsg = nullptr;
  const char *sql = "CREATE TABLE IF NOT EXISTS data("
                    "id VARCHAR(500) PRIMARY KEY,"
                    "page_content TEXT,"
                    "exp_timestamp INTEGER);";

  int exit = sqlite3_exec(db.get(), sql, NULL, 0, &errMsg);

  if (exit != SQLITE_OK) {
    throw std::runtime_error(errMsg);
    sqlite3_free(errMsg);
  } else {
    std::cout << "Table created Successfully" << std::endl;
  }
}

std::optional<RowData> SqlClient::query(const std::string &port_url) {
  int rc = 0;
  std::unique_ptr<Statement> statement;

  for (int init_attempts = 0; init_attempts < 2; init_attempts++) {
    try {

      std::string sql = "SELECT id, page_content, exp_timestamp FROM data "
                        "WHERE id = ?"; // Order is important here
      statement = std::make_unique<Statement>(sql, db.get());

      if (sqlite3_bind_text(statement->getStmt(), 1, port_url.c_str(), -1,
                            SQLITE_TRANSIENT) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db.get()));
      }
      break;
    } catch (const std::runtime_error &e) {

      if (init_attempts ==
          0) { // Necessary for creating the database on first run of script
        create_database();
      } else {
        throw;
      }
    }
  }

  int res = sqlite3_step(statement->getStmt());
  if (res == SQLITE_ROW) { // Means cache hit depending on duration
    int expiration = sqlite3_column_int64(
        statement->getStmt(), static_cast<long>(ROW_DEF::EXP_TIMESTAMP));
    const unsigned char *content = sqlite3_column_text(
        statement->getStmt(), static_cast<int>(ROW_DEF::PAGE_CONTENT));
    if (!content) {
      throw std::runtime_error("Failed to parse content from url");
    }
    std::string result{reinterpret_cast<const char *>(content)};
    return RowData{
        .id = port_url, .exp_timestamp = expiration, .content = result};
  } else {
    return nullopt;
  }
}

void SqlClient::persistRowData(const RowData rowData,
                               const ReturnStatus status) {

  std::string sql;
  if (status == CACHE_REFRESH) {

    sql = "UPDATE data SET  page_content = ?, exp_timestamp = ?  WHERE id = ?";
  } else if (status == CACHE_MISS) {
    sql = "INSERT INTO data (page_content, exp_timestamp, id) values (?, ?, ?)";
  } else {
    throw std::invalid_argument("Invalid argument for persistRowData");
  }

  int rc;
  Statement statement(sql, db.get());

  // page_content
  rc = sqlite3_bind_text(statement.getStmt(), 1, rowData.content.c_str(), -1,
                         SQLITE_TRANSIENT);
  check_rc(rc);
  // exp_timestamp
  rc = sqlite3_bind_int64(statement.getStmt(), 2, rowData.exp_timestamp);
  check_rc(rc);

  // id
  rc = sqlite3_bind_text(statement.getStmt(), 3, rowData.id.c_str(), -1,
                         SQLITE_TRANSIENT);
  check_rc(rc);
  if (sqlite3_step(statement.getStmt()) != SQLITE_DONE) {
    throw std::runtime_error("Could not cache data in local storage");
  }
}

ReturnData Db::read(const std::string &port_url) {
  std::optional<RowData> rowData = sqlClient.query(port_url);

  std::string result;
  ReturnStatus status;

  if (rowData) {
    auto current = std::chrono::duration_cast<std::chrono::seconds>(
                       time_getter.now().time_since_epoch())
                       .count();
    if (rowData->exp_timestamp < current) {
      result = update(port_url);
      status = CACHE_REFRESH;
    } else {
      result = rowData->content;
      status = CACHE_HIT;
    }
  } else {

    result = insert(port_url);
    status = CACHE_MISS;
  }
  ReturnData data = {.status = status, .content = result};
  return data;
}

std::chrono::system_clock::time_point TimeGetter::now() const {
  return std::chrono::system_clock::now();
}
Db::Db(int minute_cache_duration, iSqlClient &sqlClient,
       iHttpClient &http_client, iTimeGetter &time_getter)
    : expiration_duration(std::chrono::minutes{minute_cache_duration}),
      sqlClient(sqlClient), http_client(http_client), time_getter(time_getter) {
}

void SqlClient::check_rc(int rc) {
  if (rc != SQLITE_OK) {
    throw std::runtime_error(sqlite3_errmsg(db.get()));
  }
}
RowData Db::generateRowData(const std::string &port_url) {

  auto future = time_getter.now() + this->expiration_duration;

  long expiration_timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                                  future.time_since_epoch())
                                  .count();

  std::string page_content = http_client.read_url(port_url);
  return RowData{.id = port_url,
                 .exp_timestamp = expiration_timestamp,
                 .content = page_content};
}

std::string Db::insert(const std::string &port_url) {

  RowData rowData = generateRowData(port_url);
  sqlClient.persistRowData(rowData, CACHE_MISS);
  return rowData.content;
}

std::string Db::update(const std::string &port_url) {

  RowData rowData = generateRowData(port_url);

  sqlClient.persistRowData(rowData, CACHE_REFRESH);
  return rowData.content;
}
int Db::testIt() { return sqlClient.test(); }

Statement::Statement(std::string sqlStr, sqlite3 *dbPtr)
    : sql(sqlStr), db(dbPtr) {
  sqlite3_stmt *raw = nullptr;

  int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &raw, nullptr);
  if (rc != SQLITE_OK) {
    throw std::runtime_error(sqlite3_errmsg(db));
  }
  stmt_.reset(raw);
};
std::chrono::system_clock::time_point now() {
  return std::chrono::system_clock::now();
}
