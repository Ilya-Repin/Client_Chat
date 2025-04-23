#include "crypto.h"

namespace crypto {
    Crypto::Crypto() {
        CryptoPP::AutoSeededRandomPool rng{};
        private_key_.GenerateRandomWithKeySize(rng, 1024);
    }

    Crypto::Crypto(const std::string &file_name) {
        std::ifstream key_file(file_name, std::ios::binary);
        CryptoPP::FileSource key_source(key_file, true);

        private_key_.BERDecode(key_source);

        key_file.close();
    }

    std::string Crypto::CalculatePublicKey() const {
        CryptoPP::RSA::PublicKey public_key(private_key_);
        std::string public_key_string;

        CryptoPP::StringSink public_key_string_sink(public_key_string);
        public_key.BEREncode(public_key_string_sink);

        return public_key_string;
    }

    void Crypto::SavePrivateKey(const std::string &filename) const {
        std::ofstream private_key_file(filename, std::ios::binary);
        CryptoPP::FileSink privSink(private_key_file);
        private_key_.DEREncode(privSink);
        private_key_file.close();
    }

    std::string Crypto::GenerateSessionKey() {
        CryptoPP::AutoSeededRandomPool rng;
        CryptoPP::SecByteBlock secret_key(CryptoPP::AES::DEFAULT_KEYLENGTH);
        rng.GenerateBlock(secret_key, secret_key.size());

        std::string key_string;
        CryptoPP::StringSource(secret_key, secret_key.size(), true,
                               new CryptoPP::HexEncoder(new CryptoPP::StringSink(key_string)));

        return key_string;
    }

    std::string Crypto::EncryptAES(const std::string &plaintext, const std::string &key) const {
        CryptoPP::SecByteBlock session_key{CryptoPP::AES::DEFAULT_KEYLENGTH};

        CryptoPP::StringSource(key, true,
                               new CryptoPP::HexDecoder(
                                   new CryptoPP::ArraySink(session_key, session_key.size())));


        std::string ciphertext;

        try {
            CryptoPP::AES::Encryption encryption(session_key);
            CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(encryption, session_key);

            CryptoPP::StringSource(plaintext, true,
                                   new CryptoPP::StreamTransformationFilter(
                                       cbcEncryption, new CryptoPP::StringSink(ciphertext)));
        } catch (const std::exception &e) {
            std::cerr << "Ошибка при шифровании: " << e.what() << std::endl;
            return "";
        }

        return ciphertext;
    }

    std::string Crypto::DecryptAES(const std::string &ciphertext, const std::string &key) const {
        CryptoPP::SecByteBlock session_key{CryptoPP::AES::DEFAULT_KEYLENGTH};
        CryptoPP::StringSource(key, true,
                               new CryptoPP::HexDecoder(
                                   new CryptoPP::ArraySink(session_key, session_key.size())));

        std::string plaintext;

        try {
            CryptoPP::AES::Decryption decryption(session_key);
            CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(decryption, session_key);

            CryptoPP::StringSource(ciphertext, true,
                                   new CryptoPP::StreamTransformationFilter(
                                       cbcDecryption, new CryptoPP::StringSink(plaintext)));
        } catch (const std::exception &e) {
            std::cerr << "Ошибка при шифровании: " << e.what() << std::endl;
            return "";
        }

        return plaintext;
    }

    std::string Crypto::EncryptRSA(const std::string &plaintext, const std::string &key) const {
        CryptoPP::RSA::PublicKey public_key;

        CryptoPP::StringSource key_source(key, true);
        public_key = CryptoPP::RSA::PublicKey();
        public_key.BERDecode(key_source);

        CryptoPP::AutoSeededRandomPool rng;

        CryptoPP::RSAES_OAEP_SHA_Encryptor encryptor(public_key);

        std::string ciphered;
        CryptoPP::StringSource ciphering(plaintext, true,
                                         new CryptoPP::PK_EncryptorFilter(
                                             rng, encryptor, new CryptoPP::StringSink(ciphered)));

        std::string encoded_ciphered;
        CryptoPP::StringSource encoding(ciphered, true,
                                        new CryptoPP::Base64Encoder(new CryptoPP::StringSink(encoded_ciphered)));

        return encoded_ciphered;
    }

    std::string Crypto::DecryptRSA(const std::string &ciphertext) const {
        CryptoPP::AutoSeededRandomPool rng;
        std::string decoded_ciphered;

        CryptoPP::StringSource decoding(ciphertext, true,
                                        new CryptoPP::Base64Decoder(new CryptoPP::StringSink(decoded_ciphered)));

        std::string decrypted;
        CryptoPP::RSAES_OAEP_SHA_Decryptor decryptor(private_key_);

        CryptoPP::StringSource decrypting(decoded_ciphered, true,
                                          new CryptoPP::PK_DecryptorFilter(rng, decryptor,
                                                                           new CryptoPP::StringSink(decrypted)));

        return decrypted;
    }
}
