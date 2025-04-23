#include "app.h"


namespace app {
    void App::Run() {
        network_.Send(name_ + "@" + token_ + "\r\n\r\n");

        std::string ok = network_.Receive();
        if (ok != "Server@OK\r\n\r\n") {
            throw std::runtime_error("Conection failed");
        }

        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_int_distribution<> distribution(0, 10000);

        random_number_ = distribution(generator);

        std::cout << "\033[33mЧАТ ГОТОВ К РАБОТЕ\033[0m" << std::endl;

        try {
              user_state_.LoadState("state-" + name_);
          } catch(...) {
              std::cerr << "Не удалось загрузить состояние..." << std::endl;
          }

        StartChat();
    }

    void App::StartChat() {
        std::thread io_thread([this]() {
            network_.GetIOService().run();
        });

        network_.GetIOService().post([this]() {
            ReceiveLoop();
        });

        std::string input_text;
        while (true) {
            std::getline(std::cin, input_text);

            if (input_text == "/exit") {
                std::cout << "\033[33mСОСТОЯНИЕ СОХРАНЕНО В ФАЙЛ\033[0m" << std::endl;
                user_state_.SaveState("state-" + name_);
                break;
            }

            std::string user = input_text.substr(0, input_text.find('@'));

            auto state = user_state_.GetState(user);

            if (!state.has_value()) {
                network_.Send(user + "@" + std::to_string(random_number_) + "\r\n\r\n");
                user_state_.SetState(user, STATUS::AWAIT_NUM, "");

                std::cerr << "Отсутствует соединение с данным пользователем, подключение..." << std::endl;
            } else if (state->status == STATUS::READY) {
                auto key = state->session_key;

                network_.Send(
                    user + "@" + crypto_.EncryptAES(input_text.substr(input_text.find('@') + 1), key) + "\r\n\r\n");
            }
        }

        std::cout << "\033[33mЧАТ ЗАВЕРШАЕТ РАБОТУ\033[0m" << std::endl;
        io_thread.join();
        network_.GetIOService().stop();
        std::exit(EXIT_SUCCESS);
    }

    void App::ReceiveLoop() {
        while (true) {
            try {
                std::string received = network_.Receive();

                std::string from = received.substr(0, received.find('@'));
                std::string message = received.substr(received.find('@') + 1);

                std::optional<State> state = user_state_.GetState(from);

                if (!state.has_value()) {
                    network_.Send(from + "@" + std::to_string(random_number_) + "\r\n\r\n");

                    if (int num = std::stoi(message); random_number_ > num) {
                        network_.Send(from + "@SYN\r\n\r\n");
                        user_state_.SetState(from, STATUS::AWAIT_ACK, "");
                    } else {
                        user_state_.SetState(from, STATUS::AWAIT_SYN, "");
                    }
                } else if (state->status == STATUS::READY) {
                    auto key = state->session_key;

                    std::string decrypted = crypto_.DecryptAES(message.substr(0, message.length() - 4), key);
                    std::cout << boost::str(boost::format("\n\033[32m%s:\033[0m ") % from) << decrypted << std::endl;
                } else if (state->status == STATUS::AWAIT_NUM) {
                    auto temp = received.substr(received.find('@') + 1);
                    int num = std::stoi(temp.substr(0, temp.size() - 4));

                    if (random_number_ > num) {
                        network_.Send(from + "@SYN\r\n\r\n");

                        user_state_.SetStatus(from, STATUS::AWAIT_ACK);
                    } else {
                        user_state_.SetStatus(from, STATUS::AWAIT_SYN);
                    }
                } else if (state->status == STATUS::AWAIT_SYN) {
                    if (message == "SYN\r\n\r\n") {
                        network_.Send(from + "@ACK\r\n\r\n");
                        user_state_.SetStatus(from, STATUS::AWAIT_PUB_KEY);
                    } else {
                        std::cerr << "NOT SYN! " << message.size() << " " << message << std::endl;
                    }
                } else if (state->status == STATUS::AWAIT_ACK) {
                    if (message == "ACK\r\n\r\n") {
                        auto pub = crypto_.CalculatePublicKey();

                        network_.Send(from + "@" + pub + "\r\n\r\n");
                        user_state_.SetStatus(from, STATUS::AWAIT_SESSION_KEY);
                    } else {
                        std::cerr << "NOT ACK! " << message.size() << " " << message << std::endl;
                    }
                } else if (state->status == STATUS::AWAIT_PUB_KEY) {
                    auto pub_temp = received.substr(received.find("@") + 1); // ???
                    auto pub = pub_temp.substr(0, pub_temp.size() - 4);
                    std::string session_key = crypto_.GenerateSessionKey();

                    std::string encrypted_session_key = crypto_.EncryptRSA(session_key, pub);
                    std::string test = crypto_.DecryptRSA(encrypted_session_key);

                    std::string encrypted_test = crypto_.
                            EncryptAES("4 8 15 16 23 42", test);

                    network_.Send(from + "@" + encrypted_session_key + "\r\n\r\n");

                    user_state_.SetSessionKey(from, session_key);
                    user_state_.SetStatus(from, STATUS::AWAIT_TEST_NUMS);
                } else if (state->status == STATUS::AWAIT_SESSION_KEY) {
                    auto temp = received.substr(received.find("@") + 1);
                    user_state_.SetSessionKey(from, crypto_.DecryptRSA(temp.substr(0, temp.size() - 4)));

                    std::string encrypted_test = crypto_.
                            EncryptAES("4 8 15 16 23 42", user_state_.GetState(from)->session_key);

                    network_.Send(from + "@" + encrypted_test + "\r\n\r\n");

                    user_state_.SetStatus(from, STATUS::AWAIT_TEST_RESPONSE);
                } else if (state->status == STATUS::AWAIT_TEST_NUMS) {
                    auto temp = received.substr(received.find("@") + 1);

                    std::string decr_test = crypto_.DecryptAES(
                        temp.substr(0, temp.size() - 4), state->session_key);

                    if (decr_test == "4 8 15 16 23 42") {
                        std::string encr_response = crypto_.EncryptAES("108", state->session_key);
                        network_.Send(from + "@" + encr_response + "\r\n\r\n");

                        user_state_.SetStatus(from, STATUS::AWAIT_OK);
                    } else {
                        std::cerr << "Ошибка при проверке подключения" << std::endl;
                    }
                } else if (state->status == STATUS::AWAIT_TEST_RESPONSE) {
                    auto temp = received.substr(received.find("@") + 1);
                    std::string decrypted_response = crypto_.DecryptAES(
                        temp.substr(0, temp.size() - 4), state->session_key);

                    if (decrypted_response == "108") {
                        std::cout << boost::str(boost::format("\n\033[32mУстановлено соединение с %s\033[0m ") % from)
                                << std::endl;
                        network_.Send(from + "@OK\r\n\r\n");
                        user_state_.SetStatus(from, STATUS::READY);
                    } else {
                        std::cerr << "Ошибки при проверке шифрования" << std::endl;
                    }
                } else if (state->status == STATUS::AWAIT_OK) {
                    if (received.substr(received.find("@")) == "@OK\r\n\r\n") {
                        std::cout << boost::str(boost::format("\n\033[32mУстановлено соединение с %s\033[0m ") % from)
                                << std::endl;
                    }

                    user_state_.SetStatus(from, STATUS::READY);
                }
            } catch (const std::exception &e) {
                std::cerr << e.what() << std::endl;

                break;
            }

        }
    }
}
