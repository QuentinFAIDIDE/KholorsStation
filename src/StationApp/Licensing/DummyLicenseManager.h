#pragma once
#include "juce_core/juce_core.h"
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
    static std::optional<UserDataAndKey> getUserDataAndKeyFromDisk();

    static void writeUserDataAndKeyToDisk(std::optional<UserDataAndKey>);

    static juce::File ensureKholorsFolderExists();

    /**
     * @brief Assert if the key is valid with the most basic of all tests.
     *
     * @param usernameAndLicense Object containing email, username and key
     * @return true the key is valid according to test 1
     * @return false the key is not valid according to test 1
     */
    static bool isKeyValid(UserDataAndKey usernameAndLicense);

    /**
     * @brief Parse the license key and decode it to bytes.
     *
     * @param usernameAndLicense Object containing email, username and key
     * @return std::vector<unsigned char> a vector containing the license key
     */
    static std::vector<unsigned char> getLicenseKeyBytes(UserDataAndKey usernameAndLicense);

    static std::string bytesToHex(std::vector<unsigned char>);
    static std::string bytesToHex(unsigned char *, size_t);
};