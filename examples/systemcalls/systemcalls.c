#include "systemcalls.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/

bool do_system(const char *cmd)
{
    int ret = system(cmd);

    if (ret == -1) return false;
    if (WIFEXITED(ret) && WEXITSTATUS(ret) == 0) return true;

    return false;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    va_end(args);

    pid_t pid = fork();

    if (pid == -1) // Fork failed
    {
        return false;
    }
    else if (pid == 0)
    {
        execv(command[0], command);
        exit(EXIT_FAILURE); // We shouldn't be able to reach this point
    }
    else
    {
        int status;
        if (waitpid(pid, &status, 0) == -1) // waitpid failed
        {
            return false;
        }

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        {
            return true;
        }
    }

    return false;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    va_end(args);

    pid_t pid = fork();

    if (pid == -1) // Fork failed
    {
        return false;
    }
    else if (pid == 0)
    {
        // child process: Open the output file for redirection
        // 0644 provides Read/Write for owner, and Read for group/others
        int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0)
        {
            exit(EXIT_FAILURE);
        }

        // redirect standard output (stdout) to the file descriptor
        if (dup2(fd, STDOUT_FILENO) < 0)
        {
            close(fd);
            exit(EXIT_FAILURE);
        }

        // close the file descriptor as it is no longer needed after dup2
        close(fd);

        // execute the command
        execv(command[0], command);
        exit(EXIT_FAILURE);
    }
    else
    {
        // parent process: Wait for the child to finish
        int status;
        if (waitpid(pid, &status, 0) == -1) // waitpid failed
        {
            return false;
        }

        // check if child exited normally and with an exit code of 0
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        {
            return true;
        }
    }

    return false;
}
