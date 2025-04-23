#pragma once

#include <iomanip>
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/format.hpp>

namespace network {
  using boost::asio::ip::tcp;
  using boost::asio::streambuf;

  class HttpPostClient {
  public:
    HttpPostClient(const std::string &hostname, const std::string &api_path)
      : host(hostname), path(api_path), io_context(), socket(io_context) {
    }

    std::string send_request(const std::string &name, const std::string &password);

  private:
    std::string url_encode(const std::string &str);

    std::string make_http_post_request(const std::string &host,
                                        const std::string &path,
                                        const std::string &login,
                                        const std::string &password);

    std::string read_response();

    std::string host;
    std::string path;
    boost::asio::io_context io_context;
    tcp::socket socket;

  };
}