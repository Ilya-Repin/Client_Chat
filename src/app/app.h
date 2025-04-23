#pragma once

#include <iostream>
#include "crypto/crypto.h"
#include "network/network.h"
#include <random>
#include <shared_mutex>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <thread>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/optional.hpp>

namespace app {
    enum STATUS {
        AWAIT_NUM, AWAIT_SYN, AWAIT_ACK, AWAIT_PUB_KEY, AWAIT_SESSION_KEY, AWAIT_TEST_NUMS, AWAIT_TEST_RESPONSE,
        AWAIT_OK, READY
    };

    struct State {
        STATUS status;
        std::string session_key;

        template <class Archive>
        void serialize(Archive &ar, const unsigned int version) {
            ar &status;
            ar &session_key;
        }
    };

    class UserState {
    public:
        void ClearState(const std::string &name) {
            std::lock_guard<std::mutex> lock(mutex_);

            if (user_state_.contains(name)) {
                user_state_.erase(name);
            }
        }

        std::optional<State> GetState(const std::string &name) {
            std::lock_guard<std::mutex> lock(mutex_);

            if (user_state_.find(name) == user_state_.end()) {
                return std::nullopt;
            }

            return user_state_.at(name);
        }

        void SetState(const std::string &name, STATUS status, const std::string &session_key) {
            std::lock_guard<std::mutex> lock(mutex_);

            user_state_[name] = {status, session_key};
        }

        void SetStatus(const std::string &name, STATUS status) {
            std::lock_guard<std::mutex> lock(mutex_);

            user_state_[name].status = status;
        }

        void SetSessionKey(const std::string &name, std::string session_key) {
            std::lock_guard<std::mutex> lock(mutex_);

            user_state_[name].session_key = session_key;
        }

        void SaveState(const std::string &filename) {
            std::lock_guard<std::mutex> lock(mutex_);
            std::ofstream ofs(filename);
            if (ofs.is_open()) {
                boost::archive::text_oarchive oa(ofs);
                oa << *this;
            }
        }

        void LoadState(const std::string &filename) {
            std::lock_guard<std::mutex> lock(mutex_);
            std::ifstream ifs(filename);
            if (ifs.is_open()) {
                boost::archive::text_iarchive ia(ifs);
                ia >> *this;
            }
        }

    private:
        friend class boost::serialization::access;

        template <class Archive>
        void serialize(Archive &ar, const unsigned int version) {
            ar &user_state_;
        }

        std::unordered_map<std::string, State> user_state_;
        std::mutex mutex_;
    };


    class App {
    public:
        App(const std::string &host, int port, const std::string& pk_filename, const std::string &name,
            const std::string &token) : network_(host, port), crypto_(pk_filename), name_(name), token_(token) {
        }

        void Run();

    private:

        void StartChat();

        void ReceiveLoop();

        std::string token_;
        std::string name_;
        int random_number_;

        network::Network network_;
        crypto::Crypto crypto_;

        UserState user_state_;
    };
}
