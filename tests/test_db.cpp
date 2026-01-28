
#include "db.hpp"
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <fakeit.hpp>
#include <optional>

using namespace fakeit;

TEST_CASE("Db Class Functionality", "[db_class]") {

  Mock<iSqlClient> sql_mock;
  Mock<iTimeGetter> time_mock;
  Mock<iHttpClient> http_mock;
  const std::string fake_url = "https://www.google.com";
  const std::string fake_content = "Hello World!";
  constexpr int exp_duration_mins = 5;

  auto time_point =
      std::chrono::system_clock::time_point{std::chrono::hours{13}};

  auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                       time_point.time_since_epoch())
                       .count();

  constexpr long three_minutes = 3 * 60;

  auto expired = timestamp - three_minutes;
  auto not_expired = timestamp + three_minutes;

  When(Method(http_mock, read_url)).AlwaysReturn(fake_content);
  When(Method(time_mock, now)).AlwaysReturn(time_point);
  When(Method(sql_mock, persistRowData)).AlwaysReturn();

  Db db(exp_duration_mins, sql_mock.get(), http_mock.get(), time_mock.get());

  SECTION("Read Miss Leads to Insert") {

    When(Method(sql_mock, query)).Return(std::nullopt);
    ReturnData res = db.read(fake_url);
    Verify(Method(http_mock, read_url)).Exactly(1);
    REQUIRE(res.content == fake_content);
    REQUIRE(res.status == CACHE_MISS);
  }

  SECTION("Read Hit Returns Read Without Persisting") {
    RowData mock_data = {
        .id = fake_url, .exp_timestamp = not_expired, .content = fake_content};

    When(Method(sql_mock, query)).Return(mock_data);

    Verify(Method(http_mock, read_url)).Exactly(0);

    ReturnData res = db.read(fake_url);

    REQUIRE(res.content == fake_content);
    REQUIRE(res.status == CACHE_HIT);

    Verify(Method(sql_mock, persistRowData)).Exactly(0);
  }

  SECTION("Read Hit Updates Obsolete Data") {

    RowData mock_data = {
        .id = fake_url, .exp_timestamp = expired, .content = fake_content};

    When(Method(sql_mock, query)).Return(mock_data);

    ReturnData res = db.read(fake_url);

    auto future = time_point + std::chrono::minutes{exp_duration_mins};
    long expiration_timestamp =
        std::chrono::duration_cast<std::chrono::seconds>(
            future.time_since_epoch())
            .count();

    Verify(Method(http_mock, read_url)).Exactly(1);

    Verify(Method(sql_mock, persistRowData)
               .Matching([expiration_timestamp](const RowData r,
                                                ReturnStatus status) {
                 return r.exp_timestamp == expiration_timestamp &&
                        status == CACHE_REFRESH;
               }))
        .Once();
  }
}
