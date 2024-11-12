#include "DummyLicenseManager.h"
#include "ExecCmd.h"
#include <cstring>
#include <optional>
#include <stdexcept>

#define VALID_LICENSE_KEY "wdgXY-Wsj9A-BKuM7-RV6Z2-8aWeq-K1qJz"
#define VALID_LICENSE_USERNAME "Quentin Faidide"
#define VALID_LICENSE_MAIL "quentin.faidide@gmail.com"

void testBasicLicensing()
{
    UserDataAndKey testId{VALID_LICENSE_USERNAME, VALID_LICENSE_KEY, VALID_LICENSE_MAIL};
    if (!DummyLicenseManager::isKeyValid(testId))
    {
        throw std::runtime_error("Key was not valid");
    }
}

void testLicenseReadWriteDelete()
{
    UserDataAndKey testId{VALID_LICENSE_USERNAME, VALID_LICENSE_KEY, VALID_LICENSE_MAIL};
    DummyLicenseManager::writeUserDataAndKeyToDisk(testId);
    auto newLicenseKey = DummyLicenseManager::getUserDataAndKeyFromDisk();
    DummyLicenseManager::writeUserDataAndKeyToDisk(std::nullopt);
    if (!newLicenseKey.has_value())
    {
        throw std::runtime_error("unable to read license just written");
    }
    if (testId.licenseKey != newLicenseKey->licenseKey || testId.email != newLicenseKey->email ||
        testId.username != newLicenseKey->username)
    {
        throw std::runtime_error("retrieved license key and user data don't match the ones written on disk");
    }
}

void testAlternativeFileReading()
{
    UserDataAndKey testId{VALID_LICENSE_USERNAME, VALID_LICENSE_KEY, VALID_LICENSE_MAIL};
    DummyLicenseManager::writeUserDataAndKeyToDisk(testId);
#ifdef linux
    std::string altReading = execCmd("cat ~/.config/KholorsStation/license.v1key");
#endif
#ifdef WIN32
    std::string altReading = execCmd("type %%APPDATA%%\\KholorsStation\\license.v1key");
#endif
    DummyLicenseManager::writeUserDataAndKeyToDisk(std::nullopt);
    // there should be three tokens
    int i = 0;
    char *token = strtok(altReading.data(), "\n");
    while (token != NULL)
    {
        if (i == 0)
        {
            if (strcmp(token, VALID_LICENSE_MAIL) != 0)
            {
                throw std::runtime_error("Invalid token 1: " + std::string(token));
            }
        }
        if (i == 1)
        {
            if (strcmp(token, VALID_LICENSE_USERNAME) != 0)
            {
                throw std::runtime_error("Invalid token 2: " + std::string(token));
            }
        }
        if (i == 2)
        {
            if (strcmp(token, VALID_LICENSE_KEY) != 0)
            {
                throw std::runtime_error("Invalid token 3: " + std::string(token));
            }
        }
        i++;
        token = strtok(NULL, "\n");
    }
}

void testBottomLineLicenseeParsing()
{
    std::string bottomLine = "Jean Valjean (jean@valjean.fr) | bino bie,d^()";
    auto ret = DummyLicenseManager::parseOwnerFromBottomLineOrFail(bottomLine);
    if (ret.first != "Jean Valjean" || ret.second != "jean@valjean.fr")
    {
        throw std::runtime_error("test for licensee parsing failed");
    }
}

int main(int, char *[])
{
    testBasicLicensing();
    testLicenseReadWriteDelete();
    testAlternativeFileReading();
    testBottomLineLicenseeParsing();

    return 0;
}