/* Author Alberto FDR
 *
 * Simple Debugger in C
 *
 * First we create a Fork to create two process identical except that they have different PID (Process ID)
 * and the return value of the fork() is different. Using the return value we can make the PTRACE_TRACEME
 * request to setup debugging.
 *
 * The child process is going to execute the setup_inferior method. This method setup the request PTRACE_TRACEME
 * call to setup the process as the inferior. Then the execv() system call let’s us turn our forked process into 
 * any process we want. The proces will continue and will retain it’s process ID but after the exec() call it 
 * will load a new binary image into memory and start executing from that image’s entry point (think main function 
 * though in reality it is a little more involved than this.) . After this the inferior sent a SIGTRAP signal.
 *
 * The debugger is going to execute attach_to_inferior method. This method first waits for the SIGTRAP signal
 * of the child process. Then we need to check with WIFSTOPPED that the child was stopped by a signal. We can
 * set another macros like WIFEXITED, WIFSIGNALLED...
 * 
 */

#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/reg.h>
#include <sys/ptrace.h>
#include <errno.h>

static void setup_inferior(char *const argv[])
{
    if(ptrace(PTRACE_TRACEME) < 0){
        perror("ptrace");
        _exit(1);
    }
    execvp(argv[1], argv + 1);
    perror("exec");
    _exit(1);

}

static void attach_to_inferior(int pid){
    while(1) {
        int status;
        struct user_regs_struct regs;
        
        if(waitpid(pid, &status, 0) < 0) 
            perror("waitpid");

        if(!WIFSTOPPED(status))
            break;

        if( ptrace(PTRACE_GETREGS, pid, 0, &regs) < 0)
            perror("ptrace/GETREGS");
        
        printf( "eip=0x%08X\tesp=0x%08X\tebp=0x%08X" \
                "\teax=0x%08X\tebx=0x%08X\tecx=0x%08X\tedx=0x%08X\n",
                regs.rip, regs.rsp, regs.rbp,
                regs.rax, regs.rbx, regs.rcx, regs.rdx);

        if(ptrace(PTRACE_SINGLESTEP, pid, 0, 0) < 0)
            perror("ptrace/SINGLESTEP");
    }
}

int main(int argc, char **argv)
{
    int pid;
    do {
        pid = fork();
        switch (pid) {
            case 0:   // inferior
                setup_inferior(argv);
                break;
            case -1:  // error
                break;
            default:  // debugger
                attach_to_inferior(pid);
                break;
        }
    } while (pid == -1 && errno == EAGAIN);
    
    return 0;
}


