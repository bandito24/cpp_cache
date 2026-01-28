#include <curl/curl.h>
#include <http_client.hpp>
#include <stdexcept>

HttpClient::HttpClient() : curl(curl_easy_init()) {
  if (!curl) {
    throw std::runtime_error("initalization of curl failed");
  }
}
std::string HttpClient::read_url(const std::string &port_url) {
  const char *cString = port_url.c_str();
  CURLcode res;
  CURL *curl_ptr = curl.get();
  std::string readBuffer;
  curl_easy_setopt(curl_ptr, CURLOPT_URL, cString);
  curl_easy_setopt(curl_ptr, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl_ptr, CURLOPT_WRITEDATA, &readBuffer);
  res = curl_easy_perform(curl_ptr);
  if (res != CURLE_OK) {
    throw std::runtime_error(std::string("curl error: ") +
                             curl_easy_strerror(res));
  }
  return readBuffer;
}

size_t HttpClient::WriteCallback(void *contents, size_t size, size_t nmemb,
                                 void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}
