#pragma once
#include "Base58.h"
#include "juce_core/juce_core.h"
#include "juce_cryptography/juce_cryptography.h"
#include "spdlog/spdlog.h"
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#define SOFTWARE_ID 1
#define SOFTWARE_MAJOR_VERSION 1
#define SOFTWARE_SALT "1987d7b148e2415a"

struct UserDataAndKey
{
    std::string username;
    std::string licenseKey;
    std::string email;
};

// NOTE: All license related function are inlined to make
// it just a tiny bit harder for crackers to modify this code
// in all places.

/**
 * @brief Describes a License key manager
 * that has only the most stupid checks and is
 * designed to be broken.
 */
class DummyLicenseManager
{
  public:
    /**
     * @brief Load the key data from on disk file on user home folder.
     *
     * @return std::optional<LicenseId>
     */
    static inline std::optional<UserDataAndKey> getUserDataAndKeyFromDisk()
    {
        // ensure the KholorsStation user folder exists
        auto kholorsFolder = ensureKholorsFolderExists();
        // tries to query the license file in there and eventually return
        juce::File kholorsLicenseFile = kholorsFolder.getChildFile("license.v1key");
        if (!kholorsLicenseFile.existsAsFile())
        {
            return std::nullopt;
        }

        // read the username, license key and return that
        juce::StringArray licenseFileLines;
        kholorsLicenseFile.readLines(licenseFileLines);
        if (licenseFileLines.size() < 3)
        {
            return std::nullopt;
        }
        UserDataAndKey resp;
        resp.email = licenseFileLines[0].toStdString();
        resp.username = licenseFileLines[1].toStdString();
        resp.licenseKey = licenseFileLines[2].toStdString();
        return resp;
    }

    static inline void writeUserDataAndKeyToDisk(std::optional<UserDataAndKey> userData)
    {
        auto configFolder = ensureKholorsFolderExists();
        auto licenseFile = configFolder.getChildFile("license.v1key");
        if (licenseFile.existsAsFile())
        {
            licenseFile.deleteFile();
        }
        if (userData.has_value())
        {
            licenseFile.create();
            juce::FileOutputStream output(licenseFile);
            if (output.openedOk())
            {
                output.setPosition(0);
                output.truncate();
                char linereturn = '\n';
                output.write(userData->email.data(), userData->email.size());
                output.write(&linereturn, 1);
                output.write(userData->username.data(), userData->username.size());
                output.write(&linereturn, 1);
                output.write(userData->licenseKey.data(), userData->licenseKey.size());
            }
            else
            {
                throw std::runtime_error("unable to open license file: " + licenseFile.getFullPathName().toStdString());
            }
        }
    }

    static inline juce::File ensureKholorsFolderExists()
    {
        juce::File appDataFolder =
            juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory);
        if (!appDataFolder.exists())
        {
            appDataFolder.createDirectory();
        }
        if (appDataFolder.existsAsFile())
        {
            throw std::runtime_error("Appdata folder already exists as a file");
        }
        juce::File kholorsFolder = appDataFolder.getChildFile("KholorsStation");
        if (!kholorsFolder.exists())
        {
            kholorsFolder.createDirectory();
        }
        if (kholorsFolder.existsAsFile())
        {
            throw std::runtime_error("KholorsStation folder already exists as a file");
        }
        return kholorsFolder;
    }

    /**
     * @brief Assert if the key is valid with the most basic of all tests.
     *
     * @param usernameAndLicense Object containing email, username and key
     * @return true the key is valid according to test 1
     * @return false the key is not valid according to test 1
     */
    static inline bool isKeyValid(UserDataAndKey id)
    {
        // get byte value of license key
        auto licenseKeyContent = getLicenseKeyBytes(id);

        // generate a hash from user identifiers
        std::string licenseKeyH = "m:" + id.email + ",u:" + id.username + ",s:" + std::to_string(SOFTWARE_ID) + ",m" +
                                  std::to_string(SOFTWARE_MAJOR_VERSION) + ":" + SOFTWARE_SALT;
        juce::SHA256 shaHash(licenseKeyH.data(), licenseKeyH.size());

        std::string hexLicenseKey = bytesToHex(licenseKeyContent);
        std::string hexSha = shaHash.toHexString().toStdString();

        // check that it matches the first 8 bytes
        for (size_t i = 0; i < 16; i++)
        {
            if (hexLicenseKey[i] != hexSha[i])
            {
                spdlog::error("key verification failed at hex char " + std::to_string(i));
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Parse the license key and decode it to bytes.
     *
     * @param id Object containing email, username and key
     * @return std::vector<unsigned char> a vector containing the license key
     */
    static inline std::vector<unsigned char> getLicenseKeyBytes(UserDataAndKey id)
    {
        // remove all the hyphens from the license key
        std::string licenseBase58 = "";
        for (size_t i = 0; i < id.licenseKey.size(); i++)
        {
            if (id.licenseKey[i] != '-' && id.licenseKey[i] != ' ' && id.licenseKey[i] != '\n')
            {
                if (BASE58_ALPHABET_MAP[id.licenseKey[i]] == -1)
                {
                    throw std::runtime_error("License key contains invalid characters");
                }
                licenseBase58 += id.licenseKey[i];
            }
        }
        // if the base58 string is too loog it's a good idea to abort
        // to avoid overflow of the result in the decodeBase58 code
        if (licenseBase58.size() >= 32 || licenseBase58.size() < 20)
        {
            throw std::runtime_error("License key sting has innapropriate size: " + licenseBase58);
        }
        // parse the base58 encoded bytes (512 of preallocated size should be plenty enough)
        unsigned char result[32];
        int resultlen = DecodeBase58(licenseBase58, licenseBase58.size(), result);
        if (resultlen != 22)
        {
            throw std::runtime_error("the base58 decoded key is not 22 bytes long: " + std::to_string(resultlen));
        }
        std::vector<unsigned char> resultVector;
        resultVector.resize((size_t)resultlen);
        for (size_t i = 0; i < (size_t)resultlen; i++)
        {
            resultVector[i] = result[i];
        }
        return resultVector;
    }

    static inline std::string bytesToHex(std::vector<unsigned char> a)
    {
        std::stringstream str;
        for (size_t i = 0; i < a.size(); ++i)
        {
            str << std::hex << (0xFF & a[i]);
        }
        return str.str();
    }

    static inline std::string bytesToHex(unsigned char *a, size_t size)
    {
        std::stringstream str;
        for (size_t i = 0; i < size; ++i)
        {
            str << std::hex << (0xFF & a[i]);
        }
        return str.str();
    }

    static inline std::pair<std::string, std::string> parseOwnerFromBottomLineOrFail(std::string licenseTextPart)
    {
        // try to find the first and second parenthesis
        auto parenthesisOpen = licenseTextPart.find_first_of("(", 0);
        auto parenthesisClose = licenseTextPart.find_first_of(")", 0);
        if (parenthesisOpen == string::npos || parenthesisClose == string::npos || parenthesisClose <= parenthesisOpen)
        {
            DummyLicenseManager::writeUserDataAndKeyToDisk(std::nullopt);
            throw std::runtime_error("Bottom bar text formatting error");
        }
        auto username = licenseTextPart.substr(0, parenthesisOpen - 1);
        auto mail = licenseTextPart.substr(parenthesisOpen, parenthesisClose - parenthesisOpen);
        return std::pair<std::string, std::string>(username, mail);
    }
};