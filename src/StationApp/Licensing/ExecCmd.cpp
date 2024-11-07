#include "ExecCmd.h"

#include <cstring>
#include <stdexcept>

std::string execCmd(const char *cmd)
{
    char buffer[256];
    std::string result = "";
#ifdef linux
    FILE *pipe = popen(cmd, "r");
#endif
#ifdef WIN32
    FILE *pipe = _popen(cmd, "r");
#endif
    if (!pipe)
        throw std::runtime_error("popen() failed!");
    try
    {
        while (fgets(buffer, sizeof buffer, pipe) != NULL)
        {
            result += buffer;
        }
    }
    catch (...)
    {
#ifdef linux
        pclose(pipe);
#endif
#ifdef WIN32
        _pclose(pipe);
#endif
        throw;
    }

#ifdef linux
    pclose(pipe);
#endif
#ifdef WIN32
    _pclose(pipe);
#endif

    return result;
}