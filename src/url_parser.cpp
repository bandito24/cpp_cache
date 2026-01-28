#include <format>
#include <regex>
#include <stdexcept>
#include <string>
// #include <url_parser.hpp>

std::string parseURL(const std::string &url, std::optional<std::string> port) {

  size_t schemeIndex = url.find("://");
  if (schemeIndex == std::string::npos) {
    throw std::runtime_error("Schema must be provided for url");
  }
  size_t afterScheme = schemeIndex + 3;
  size_t portIndex = url.find(":", afterScheme);
  size_t tailIndex = url.find_first_of("/?", afterScheme);

  std::string result;

  if (port) {
    result = std::format("")
  }
}
int main() {}
