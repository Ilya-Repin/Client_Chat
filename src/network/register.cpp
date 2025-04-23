#include "register.h"

namespace network {
    std::string HttpPostClient::send_request(const std::string &name, const std::string &password)  {
        boost::asio::io_service io_service;
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(host, "8080");
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        boost::asio::connect(socket, endpoint_iterator);

        std::string request = make_http_post_request(host, path, name, password);
        boost::asio::write(socket, boost::asio::buffer(request));

        return read_response();
    }

    std::string HttpPostClient::url_encode(const std::string &str)  {
        std::ostringstream encoded;
        encoded << std::hex << std::uppercase;
        for (unsigned char c : str) {
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                encoded << c;
            } else {
                encoded << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(c);
            }
        }
        return encoded.str();
    }

    std::string HttpPostClient::make_http_post_request(const std::string &host, const std::string &path,
    const std::string &login, const std::string &password)  {
        std::string encoded_login = url_encode(login);
        std::string encoded_password = url_encode(password);

        std::string url = path + "?name=" + encoded_login;

        std::string body = encoded_password;

        std::string request = boost::str(boost::format(
                                           "POST %s HTTP/1.1\r\n"
                                           "Host: %s\r\n"
                                           "Content-Type: application/x-www-form-urlencoded\r\n"
                                           "Content-Length: %d\r\n"
                                           "\r\n"
                                           "%s\r\n"
                                         ) % url % host % body.size() % body);

        return request;
    }


    std::string HttpPostClient::read_response()  {
        streambuf response;
        boost::system::error_code error;
        boost::asio::read_until(socket, response, "\0", error);

        if (error == boost::asio::error::eof) {
            std::cerr << "Connection closed by server." << std::endl;
            return "";
        }

        if (error) {
            std::cerr << "Error reading response: " << error.message() << std::endl;
            return "";
        }

        std::istream response_stream(&response);
        std::string line;
        std::getline(response_stream, line);

        if (line.find("HTTP/1.1 401") != std::string::npos) {
            throw std::runtime_error("Неправильный пароль!");
        }

        if (line.find("HTTP/1.1 400") != std::string::npos) {
            throw std::runtime_error("Неправильный формат!");
        }

        if (line.find("HTTP/1.1 201") != std::string::npos) {
            std::cout << "Пользователь зарегистрирован!" << std::endl;
        }

        std::string token;
        while (std::getline(response_stream, line)) {
            token = line;
        }

        return token;
    }
}