#pragma once
#include <curl/curl.h>
#include <memory>
#include <string>

struct curlDeleterFunction {
  void operator()(CURL *curl) {
    if (curl) {
      curl_easy_cleanup(curl);
    }
  }
};

struct iHttpClient {
  virtual std::string read_url(const std::string &port_url) = 0;
  virtual ~iHttpClient() = default;
};

class HttpClient : public iHttpClient {
private:
  std::unique_ptr<CURL, curlDeleterFunction> curl;
  static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                              void *userp);

public:
  HttpClient();
  std::string read_url(const std::string &port_url) override;
};
