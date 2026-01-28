#include "db.hpp"
#include "http_client.hpp"
#include <ada.h>
#include <exception>
#include <iostream>
#include <ostream>
#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include <unistd.h>
using namespace std;
#include <curl/curl.h>
using namespace std::chrono;

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}

int main(int argc, char *argv[]) {

  try {
    int opt;
    std::optional<string> port, addr;
    std::optional<int> duration_mins;
    while ((opt = getopt(argc, argv, "p:a:t:")) != -1) {
      switch (opt) {
      case 'p':
        port = optarg;
        break;
      case 'a':
        addr = optarg;
        break;
      case 't':
        duration_mins = std::stoi(optarg);
        break;
      default:
        std::cerr << "Unknown option\n";
        return 1;
      }
    }
    if (!addr) {
      throw std::runtime_error("Must Provide Url Address For Cache");
    }
    auto url = ada::parse<ada::url_aggregator>(*addr);
    if (!url) {
      throw std::runtime_error("Invalid Url Provided");
    }
    if (!duration_mins) {
      duration_mins = 45;
    }
    // std::cout << url->to_string() << std::endl;

    if (port) {
      url->set_port(*port);
    } else if (url->get_port().empty()) {
      std::string_view protocol = url->get_protocol();
      if (protocol.compare("https:") == 0) {
        std::cout << "Defaulting to SSL/TLS port 443" << std::endl;
        url->set_port("443");
      } else if (protocol.compare("http:") == 0) {
        std::cout << "Defaulting to HTTP port 80" << std::endl;
        url->set_port("80");
      } else {
        throw std::runtime_error("Please provide a port argument");
      }
    }
    std::string search_arg = std::string{url->get_href()};
    SqlClient sqlClient(Db::DEFAULT_FILENAME);
    TimeGetter time_getter{};
    HttpClient http_client{};
    Db database(*duration_mins, sqlClient, http_client, time_getter);
    ReturnData data = database.read(search_arg);
    std::cout << data.content << std::endl;
    if (data.status == CACHE_MISS) {
      std::cout << "CACHE MISS" << std::endl;
    } else if (data.status == CACHE_HIT) {
      std::cout << "CACHE HIT" << std::endl;
    } else {
      std::cout << "CACHE REFRESH" << std::endl;
    }

    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "An unknown error occurred";
  }
}
