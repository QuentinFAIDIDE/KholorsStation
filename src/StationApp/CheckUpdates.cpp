#include "CheckUpdates.h"

#include "juce_core/juce_core.h"
#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

#define GIT_REPO_RELEASE_URL "https://api.github.com/repos/QuentinFAIDIDE/KholorsStation/releases"
#define GIT_RELEASE_HEADERS "Accept: application/vnd.github+json\nX-GitHub-Api-Version: 2022-11-28\n"
#define GIT_REQUEST_TIMEOUT_MS 2000

std::optional<std::string> getLastGitReleaseTag()
{
    try
    {
        juce::URL url(GIT_REPO_RELEASE_URL);

        juce::String headers = GIT_RELEASE_HEADERS;
        int statusCode = 0;

        std::unique_ptr<juce::InputStream> stream =
            url.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                      .withExtraHeaders(headers)
                                      .withConnectionTimeoutMs(GIT_REQUEST_TIMEOUT_MS)
                                      .withStatusCode(&statusCode)
                                      .withNumRedirectsToFollow(5));

        if (stream != nullptr)
        {
            juce::String jsonString = stream->readEntireStreamAsString();
            auto json = juce::JSON::parse(jsonString);

            if (json.isArray())
            {
                auto releases = json.getArray();
                if (releases->size() > 0)
                {
                    juce::var firstRelease = releases->getFirst();
                    if (firstRelease.isObject())
                    {
                        auto tagName = firstRelease["tag_name"];
                        if (tagName.isString())
                        {
                            return tagName.toString().toStdString();
                        }
                        else
                        {
                            spdlog::warn("tag_name of first github release is not a string");
                        }
                    }
                    else
                    {
                        spdlog::warn("first element of json array retrieved from github releases is not an object");
                    }
                }
                else
                {
                    spdlog::warn("json array retrieved from github releases has no elements");
                }
            }
            else
            {
                spdlog::warn("json retrieved from github releases is not an array");
            }
        }
        else
        {
            spdlog::warn("stream from Github releases URL was unable to be opened");
        }
    }
    catch (std::exception &e)
    {
        spdlog::error("Unable to query latest git tag: {}", e.what());
    }
    catch (...)
    {
        spdlog::error("Unable to query latest git tag");
    }
    spdlog::warn("No Github release found for KholorsStation");
    return std::nullopt;
}

SemanticVersion::SemanticVersion(int major, int minor, int patch) : major(major), minor(minor), patch(patch)
{
}

SemanticVersion::SemanticVersion(std::string version)
{
    std::string majorStr;
    std::string minorStr;
    std::string patchStr;

    if (version.empty() || !version.starts_with("v"))
    {
        throw std::invalid_argument("invalid version string passed to SemanticVersion");
    }

    size_t firstDot = version.find('.');
    if (firstDot == std::string::npos)
    {
        throw std::invalid_argument("Invalid version string (no first dot)");
    }
    majorStr = version.substr(1, firstDot - 1);

    size_t secondDot = version.find('.', firstDot + 1);
    if (secondDot == std::string::npos)
    {
        throw std::invalid_argument("Invalid version string (no second dot)");
    }
    minorStr = version.substr(firstDot + 1, secondDot - firstDot - 1);
    patchStr = version.substr(secondDot + 1);

    major = std::stoi(majorStr);
    minor = std::stoi(minorStr);
    // interesting behaviour of stoi is to truncate after first non numeric character
    // which makes our developement version tags patch number pass this without error
    patch = std::stoi(patchStr);
}

bool SemanticVersion::operator<(const SemanticVersion &other) const
{
    if (major != other.major)
    {
        return major < other.major;
    }
    if (minor != other.minor)
    {
        return minor < other.minor;
    }
    return patch < other.patch;
}

bool SemanticVersion::operator==(const SemanticVersion &other) const
{
    return major == other.major && minor == other.minor && patch == other.patch;
}

bool SemanticVersion::operator!=(const SemanticVersion &other) const
{
    return !(*this == other);
}

bool SemanticVersion::operator>(const SemanticVersion &other) const
{
    return !(*this < other) && !(*this == other);
}

bool SemanticVersion::operator>=(const SemanticVersion &other) const
{
    return (*this > other) || (*this == other);
}

bool SemanticVersion::operator<=(const SemanticVersion &other) const
{
    return (*this < other) || (*this == other);
}

bool checkIfUpdateAvailable()
{
    spdlog::info("current version: {}", GIT_DESCRIBE_VERSION);
    auto optVersionTag = getLastGitReleaseTag();
    if (optVersionTag.has_value())
    {
        spdlog::info("latest release version on GitHub: {}", optVersionTag.value());
        try
        {
            SemanticVersion version(optVersionTag.value());
            SemanticVersion currentVersion(GIT_DESCRIBE_VERSION);
            if (version > currentVersion)
            {
                spdlog::warn("A new version of KholorsStation is available. Please update to version {} from your "
                             "package manager or github.",
                             optVersionTag.value());
                return true;
            }
        }
        catch (std::exception &e)
        {
            spdlog::info("unable to parse version, error is {}, it is probably due to a dev build or invalid github "
                         "release tag format",
                         e.what());
        }
    }
    return false;
}