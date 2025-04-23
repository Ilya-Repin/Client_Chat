#include "network.h"

namespace network {
    boost::asio::io_service &Network::GetIOService() {
        return io_service_;
    }

    void Network::Send(const std::string &data) {
        boost::asio::write(socket_, boost::asio::buffer(data));
    }

    std::string Network::Receive() {
        boost::asio::streambuf buffer;
        boost::system::error_code error;

        read_until(socket_, buffer, "\r\n\r\n", error);

        if (error == boost::asio::error::eof) {
            throw std::runtime_error("Собеседник разорвал подключение");
        }

        if (error) {
            throw boost::system::system_error(error);
        }

        std::string recieved{std::istreambuf_iterator<char>(&buffer), std::istreambuf_iterator<char>()};

        return recieved;
    }

    void Network::Close() {
        socket_.close();
    }
}
