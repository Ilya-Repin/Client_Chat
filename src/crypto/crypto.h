#pragma once

#include <cryptopp/rsa.h>
#include <cryptopp/files.h>
#include <cryptopp/osrng.h>
#include <cryptopp/base64.h>
#include <cryptopp/hex.h>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/cryptlib.h>
#include <cryptopp/filters.h>

namespace crypto {
    class Crypto {
    public:
        Crypto();

        explicit Crypto(const std::string &file_name);

        ~Crypto() = default;

        std::string CalculatePublicKey() const;

        void SavePrivateKey(const std::string &filename) const;

        std::string GenerateSessionKey();

        std::string EncryptAES(const std::string &plaintext, const std::string &key) const;

        std::string DecryptAES(const std::string &ciphertext, const std::string &key) const;

        std::string EncryptRSA(const std::string &plaintext, const std::string &key) const;

        std::string DecryptRSA(const std::string &ciphertext) const;

    private:
        CryptoPP::RSA::PrivateKey private_key_;
    };
}
