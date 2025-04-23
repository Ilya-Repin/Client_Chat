#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/chrono.hpp>
#include <boost/array.hpp>

namespace network {
    using boost::asio::ip::tcp;

    class Network {
    public:
        Network(const std::string &host, unsigned int port)
            : io_service_(),
              socket_(io_service_),
              resolver_(io_service_) {
            tcp::resolver::query query(host, std::to_string(port));
            tcp::resolver::iterator endpoint_iterator = resolver_.resolve(query);

            boost::asio::connect(socket_, endpoint_iterator);
        }

        boost::asio::io_service &GetIOService();


        void Send(const std::string &data);

        std::string Receive();

        void Close();

    private:
        boost::asio::io_service io_service_;
        tcp::socket socket_;
        tcp::resolver resolver_;
        std::mutex mutex_;
    };
}
