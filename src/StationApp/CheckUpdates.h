#pragma once

#include <optional>
#include <string>

struct SemanticVersion
{
    SemanticVersion(int major, int minor, int patch);
    SemanticVersion(std::string version);
    int major;
    int minor;
    int patch;
    bool operator<(const SemanticVersion &other) const;
    bool operator==(const SemanticVersion &other) const;
    bool operator!=(const SemanticVersion &other) const;
    bool operator>(const SemanticVersion &other) const;
    bool operator>=(const SemanticVersion &other) const;
    bool operator<=(const SemanticVersion &other) const;
};

std::optional<std::string> getLastGitReleaseTag();
bool checkIfUpdateAvailable();