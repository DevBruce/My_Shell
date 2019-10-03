#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#define BSIZE 1024


char* history[5000];
int input_cnt;
bool set_noclobber = false;

int div_cmd_by_semicolon(char* cmd_raw, char* cmd_div_by_semicolon_list[]);
int div_cmd_by_op(char* cmd_div_by_semicolon, char* cmd_div_by_op_list[]);
int tokenize(char* cmd_div_by_op, char* tokens[]);
bool amp_chk(int* div_cnt_by_op_ptr, char* cmd_div_by_op_list[]);
void run_cmd(int token_cnt, char* tokens[], bool is_bg);
void run_op(int token_cnt_list[], char* tokens_list[][50], int op_num, bool is_bg, bool set_noclobber);
void run_pipe(int token_cnt_list[], char* tokens_list[][50], int pipe_num);
void multiple_pipe(int token_cnt_list[], char* tokens_list[][50], int pipe_num);
int cmd_history(int token_cnt, char* tokens[]);
int cmd_cd(int token_cnt, char* tokens[]);


int main(void)
{
    char buf_path[BSIZE];
    char cmd_raw[BSIZE];
    char* cmd_div_by_semicolon_list[100];
    char* cmd_div_by_op_list[200];
    char* tokens[150];
    char* tokens_list[50][50];
    int token_cnt_list[50];
    int div_cnt_by_op;
    int* div_cnt_by_op_ptr = &div_cnt_by_op;
    int fd;
    input_cnt = 0;
    
    printf("\n=====================================\n");
    printf("--- Implementated Features ---\n");
    printf(" * cd\n");
    printf(" * history\n");
    printf(" * Multiple Commands (;)\n");
    printf(" * Background execution (&)\n");
    printf(" * Redirection >, >>, <\n");
    printf(" * Redirection >| (Ignore [set -o noclobber])\n");
    printf(" * Pipe |\n");
    printf(" * Multiple Pipe \n");
    printf("------------------------------\n");
    printf("  + set -o noclobber, set +o noclobber\n");
    printf("=====================================\n\n");

    while(1)
    {
        char* cur_path = getcwd(buf_path, BSIZE);
        printf("%s >>> ", cur_path);
        fgets(cmd_raw, BSIZE, stdin);
        cmd_raw[strlen(cmd_raw)-1] = '\0';  // Remove '\n'

        // Save cmd string for history
        char* history_str = malloc(sizeof(char) * 100);
        strcpy(history_str, cmd_raw);
        history[input_cnt] = history_str;
        input_cnt++;

        int div_cnt = div_cmd_by_semicolon(cmd_raw, cmd_div_by_semicolon_list);
        for(int i=0; i<div_cnt; i++)
        {

            *div_cnt_by_op_ptr = div_cmd_by_op(cmd_div_by_semicolon_list[i], cmd_div_by_op_list);
            bool amp = amp_chk(div_cnt_by_op_ptr, cmd_div_by_op_list);

            for(int j=0; j<div_cnt_by_op; j++)
            {
                int token_cnt = tokenize(cmd_div_by_op_list[j], tokens);
                token_cnt_list[j] = token_cnt;

                for(int k=0; k<token_cnt; k++)
                    tokens_list[j][k] = tokens[k];
            }

            int op_num = div_cnt_by_op / 2;
            if (op_num >= 1)
                run_op(token_cnt_list, tokens_list, op_num, amp, set_noclobber);
            else
                run_cmd(token_cnt_list[0], tokens_list[0], amp);
        }
        // -- Clear tokens --
        for(int i=0; i<50+1; i++)
        {
            for(int j=0; j<50+1; j++)
                tokens_list[i][j] = NULL;
        }
        // --- --- --- --- ---
    }
}


int div_cmd_by_semicolon(char* cmd_raw, char* cmd_div_by_semicolon_list[])
{
    int div_cnt = 0;
    char delims[] = ";";
    char* cmd;
    cmd = strtok(cmd_raw, delims);
    while(cmd != NULL)
    {
        cmd_div_by_semicolon_list[div_cnt] = cmd;
        cmd = strtok(NULL, delims);
        div_cnt++;
    }
    return div_cnt;
}


int div_cmd_by_op(char* cmd_div_by_semicolon, char* cmd_div_by_op_list[])
{
    char* cmd_div_by_semicolon_modify = malloc(sizeof(char) * 100);
    int i = 0;
    int j = 0;
    bool is_str = false;

    while (cmd_div_by_semicolon[i] != '\0')
    {
        // --- Process For classifying string ---
        if (cmd_div_by_semicolon[i] == '"' || cmd_div_by_semicolon[i] == '\'')
        {
            if (!is_str)
                is_str = true;
            else
                is_str = false;
            
            cmd_div_by_semicolon_modify[j] = ' ';
            i++;
            j++;
        }
        // -----------------------

        if (!is_str)  // Normal
        {
            if (cmd_div_by_semicolon[i] == '>')
            {
                if (cmd_div_by_semicolon[i+1] == '>')
                {
                    cmd_div_by_semicolon_modify[j] = ':';
                    cmd_div_by_semicolon_modify[j+1] = '>';
                    cmd_div_by_semicolon_modify[j+2] = '>';
                    cmd_div_by_semicolon_modify[j+3] = ':';
                    j += 3;
                    i++;
                }
                else if (cmd_div_by_semicolon[i+1] == '|')
                {
                    cmd_div_by_semicolon_modify[j] = ':';
                    cmd_div_by_semicolon_modify[j+1] = '>';
                    cmd_div_by_semicolon_modify[j+2] = '|';
                    cmd_div_by_semicolon_modify[j+3] = ':';
                    j += 3;
                    i++;
                }
                else
                {
                    cmd_div_by_semicolon_modify[j] = ':';
                    cmd_div_by_semicolon_modify[j+1] = '>';
                    cmd_div_by_semicolon_modify[j+2] = ':';
                    j += 2;
                }
            }
            else if (cmd_div_by_semicolon[i] == '<')
            {
                if (cmd_div_by_semicolon[i+1] == '<')
                {
                    cmd_div_by_semicolon_modify[j] = ':';
                    cmd_div_by_semicolon_modify[j+1] = '<';
                    cmd_div_by_semicolon_modify[j+2] = '<';
                    cmd_div_by_semicolon_modify[j+3] = ':';
                    j += 3;
                    i++;
                }
                else
                {
                    cmd_div_by_semicolon_modify[j] = ':';
                    cmd_div_by_semicolon_modify[j+1] = '<';
                    cmd_div_by_semicolon_modify[j+2] = ':';
                    j += 2;
                }
            }
            else if (cmd_div_by_semicolon[i] == '|')
            {            
                cmd_div_by_semicolon_modify[j] = ':';
                cmd_div_by_semicolon_modify[j+1] = '|';
                cmd_div_by_semicolon_modify[j+2] = ':';
                j += 2;   
            }
            else if (cmd_div_by_semicolon[i] == '&')
            {            
                cmd_div_by_semicolon_modify[j] = ':';
                cmd_div_by_semicolon_modify[j+1] = '&';
                cmd_div_by_semicolon_modify[j+2] = ':';
                j += 2;   
            }
            else
            {
                cmd_div_by_semicolon_modify[j] = cmd_div_by_semicolon[i];
            }
        }
        else
        {
            cmd_div_by_semicolon_modify[j] = cmd_div_by_semicolon[i];
        }
        i++;
        j++;
    }
    int div_cnt_by_op = 0;
    char* token_by_op = strtok(cmd_div_by_semicolon_modify, ":");
    while(token_by_op != NULL)
    {
        cmd_div_by_op_list[div_cnt_by_op] = token_by_op;
        token_by_op = strtok(NULL, ":");
        div_cnt_by_op++;
    }
    cmd_div_by_op_list[div_cnt_by_op] = NULL;
    return div_cnt_by_op;
}


void run_cmd(int token_cnt, char* tokens[], bool is_bg)
{
    pid_t pid_bg;

    if (is_bg)
    {
        pid_bg = fork();
        if (pid_bg == 0)
        {
            printf("\nRun from Background Process (PID: %d)\n", getpid());

            // Remove stdout and stderr of background process
            int fd = open("/dev/null", O_WRONLY);
            int stdout_cp = dup(1);
            int stderr_cp = dup(2);
            dup2(fd, 1);
            dup2(fd, 2);

            pid_t pid;
            if (strcmp(tokens[0], "history") == 0)
            {
                cmd_history(token_cnt, tokens);
                exit(0);
            }
            else if (strcmp(tokens[0], "cd") == 0)
            {
                cmd_cd(token_cnt, tokens);
                exit(0);
            }
            else if (strcmp(tokens[0], "ls") == 0)
            {
                pid = fork();
                if (pid == 0) 
                {
                    if (token_cnt == 1)
                        execlp("ls", "ls", NULL);
                    else
                        execvp("ls", tokens);
                }
                else
                {
                    waitpid(pid, NULL, 0);
                    exit(0);
                }
            }
            else if (strcmp(tokens[0], "cat") == 0)
            {
                pid = fork();
                if (pid == 0)
                {
                    if (token_cnt == 1)
                        execlp("cat", "cat", NULL);
                    else
                        execvp("cat", tokens);
                }
                else
                {
                    waitpid(pid, NULL, 0);
                    exit(0);
                }
            }
            else if (strcmp(tokens[0], "grep") == 0)
            {
                pid = fork();
                if (pid == 0) 
                {
                    if (token_cnt == 1)
                        execlp("grep", "grep", NULL);
                    else
                        execvp("grep", tokens);
                }
                else
                {
                    waitpid(pid, NULL, 0);
                    exit(0);
                }
            }
            else if (strcmp(tokens[0], "echo") == 0)
            {
                pid = fork();
                if (pid == 0) 
                {
                    if (token_cnt == 1)
                        execlp("echo", "echo", NULL);
                    else
                        execvp("echo", tokens);
                }
                else
                {
                    waitpid(pid, NULL, 0);
                    exit(0);
                }
            }
            else
            {
                printf("\nError: Not Implemented Command\n");
                exit(0);
            }
            dup2(stdout_cp, 1);
            dup2(stderr_cp, 2);
            close(fd);
        }
        else
        {
            waitpid(pid_bg, NULL, WNOHANG);
            sleep(1);
        }
    }
    else
    {
        pid_t pid;
        if (strcmp(tokens[0], "history") == 0)
        {
            cmd_history(token_cnt, tokens);
        }
        else if (strcmp(tokens[0], "cd") == 0)
        {
            cmd_cd(token_cnt, tokens);
        }
        else if (strcmp(tokens[0], "set") == 0 &&
                 strcmp(tokens[1], "-o") == 0 &&
                 strcmp(tokens[2], "noclobber") == 0)
        {
            set_noclobber = true;
            printf("\n** Overwrite disabled **\n");
        }
        else if (strcmp(tokens[0], "set") == 0 &&
                 strcmp(tokens[1], "+o") == 0 &&
                 strcmp(tokens[2], "noclobber") == 0)
        {
            set_noclobber = false;
            printf("\n** Overwrite enabled **\n");
        }
        else if (strcmp(tokens[0], "ls") == 0)
        {
            pid = fork();
            if (pid == 0) 
            {
                if (token_cnt == 1)
                    execlp("ls", "ls", NULL);
                else
                    execvp("ls", tokens);
            }
            else
            {
                waitpid(pid, NULL, 0);
            }
        }
        else if (strcmp(tokens[0], "cat") == 0)
        {
            pid = fork();
            if (pid == 0) 
            {
                if (token_cnt == 1)
                    execlp("cat", "cat", NULL);
                else
                    execvp("cat", tokens);
            }
            else
            {
                waitpid(pid, NULL, 0);
            }
        }
        else if (strcmp(tokens[0], "grep") == 0)
        {
            pid = fork();
            if (pid == 0) 
            {
                if (token_cnt == 1)
                    execlp("grep", "grep", NULL);
                else
                    execvp("grep", tokens);
            }
            else
            {
                waitpid(pid, NULL, 0);
            }
        }
        else if (strcmp(tokens[0], "echo") == 0)
        {
            pid = fork();
            if (pid == 0) 
            {
                if (token_cnt == 1)
                    execlp("echo", "echo", NULL);
                else
                    execvp("echo", tokens);
            }
            else
            {
                waitpid(pid, NULL, 0);
            }
        }
        else
        {
            printf("\nError: Not Implemented Command\n");
        }
    }
}


bool amp_chk(int* div_cnt_by_op_ptr, char* cmd_div_by_op_list[])
{
    for(int i=0; i<*div_cnt_by_op_ptr; i++)
    {
        if (strcmp(cmd_div_by_op_list[i], "&") == 0)
        {
            cmd_div_by_op_list[i] = " ";
            *div_cnt_by_op_ptr -= 1;
            return true;
        }
    }
    return false;
}


int tokenize(char* cmd_div_by_op, char* tokens[])
{
    int token_cnt = 0;
    char delims[] = " \t\n";
    char* token;
    token = strtok(cmd_div_by_op, delims);
    while(token != NULL)
    {
        tokens[token_cnt] = token;
        token = strtok(NULL, delims);
        token_cnt++;
    }
    tokens[token_cnt] = NULL;
    return token_cnt;
}


int cmd_history(int token_cnt, char* tokens[])
{
    if (token_cnt == 1)
    {
        printf("==== History of cmd ====\n");
        for(int i=0; i<input_cnt-1; i++)
        {
            printf("[%d] %s\n", i+1, history[i]);
        }
        printf("\n========================\n");
        return input_cnt;
    }
    return -1;
}


int cmd_cd(int token_cnt, char* tokens[])
{
    if (token_cnt == 1)
    {
        chdir(getenv("HOME"));
    }
    else if (token_cnt == 2)
    {
        if (chdir(tokens[1]) == -1)
        {
            fprintf(stderr, "**Error: chdir Failed **\n");
            return -1;
        }
    }
    return 0;
}


void run_op(int token_cnt_list[], char* tokens_list[][50], int op_num, bool is_bg, bool set_noclobber)
{
    int stdin_cp = dup(0);
    int stdout_cp = dup(1);
    int stderr_cp = dup(2);
    pid_t pid_bg;
    int dest;
    int fd;
    char* first_op = tokens_list[1][0];


    if (is_bg)
    {
        pid_bg = fork();
        if (pid_bg == 0)
        {
            printf("\nRun from Background Process (PID: %d)\n", getpid());
            if (op_num <= 1)
            {
                if (strcmp(first_op, ">") == 0 && !set_noclobber)
                {
                    dest = open(tokens_list[2][0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (dest > 0)
                    {
                        int stdout_cp = dup(1);
                        dup2(dest, 1);
                        run_cmd(token_cnt_list[0], tokens_list[0], false);
                        fflush(stdout);
                        dup2(stdout_cp, 1);
                    }
                    close(dest);
                }
                if (strcmp(first_op, ">") == 0 && set_noclobber)
                {
                    dest = open(tokens_list[2][0], O_WRONLY | O_CREAT | O_EXCL, 0644);
                    if (dest > 0)
                    {
                        int stdout_cp = dup(1);
                        dup2(dest, 1);
                        run_cmd(token_cnt_list[0], tokens_list[0], false);
                        fflush(stdout);
                        dup2(stdout_cp, 1);
                    }
                    close(dest);
                }
                else if (strcmp(first_op, ">|") == 0)
                {
                    dest = open(tokens_list[2][0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    int stdout_cp = dup(1);
                    dup2(dest, 1);
                    run_cmd(token_cnt_list[0], tokens_list[0], false);
                    fflush(stdout);
                    dup2(stdout_cp, 1);
                    close(dest);
                }
                else if (strcmp(first_op, ">>") == 0)
                {
                    dest = open(tokens_list[2][0], O_WRONLY | O_CREAT | O_APPEND, 0644);
                    int stdout_cp = dup(1);
                    dup2(dest, 1);
                    run_cmd(token_cnt_list[0], tokens_list[0], false);
                    fflush(stdout);
                    dup2(stdout_cp, 1);
                    close(dest);
                }
                else if (strcmp(first_op, "<") == 0)
                {
                    dest = open(tokens_list[2][0], O_RDONLY);
                    int stdin_cp = dup(0);
                    dup2(dest, 0);
                    run_cmd(token_cnt_list[0], tokens_list[0], false);
                    fflush(stdin);
                    dup2(stdin_cp, 0);
                    close(dest);
                }
                else if (strcmp(first_op, "|") == 0)
                {
                    run_pipe(token_cnt_list, tokens_list, op_num);
                }
            }
            else
            {
                // check all op are pipe or not
                bool op_are_all_pipe = true;
                for (int i=0; i<op_num; i++)
                {
                    if (strcmp(tokens_list[2*i+1][0], "|") != 0)
                        op_are_all_pipe = false;
                }

                if (op_are_all_pipe)
                {
                    multiple_pipe(token_cnt_list, tokens_list, op_num);
                }
            }
            exit(0);
        }
        else
        {
            waitpid(pid_bg, NULL, WNOHANG);
            sleep(1);
        }
    }
    else
    {
        if (op_num <= 1)
        {
            if (strcmp(first_op, ">") == 0 && set_noclobber)
            {
                dest = open(tokens_list[2][0], O_WRONLY | O_CREAT | O_EXCL, 0644);
                if (dest > 0)
                {
                    int stdout_cp = dup(1);
                    dup2(dest, 1);
                    run_cmd(token_cnt_list[0], tokens_list[0], false);
                    fflush(stdout);
                    dup2(stdout_cp, 1);
                }
                else  // dest == -1 (Already Exists)
                {
                    printf("\nError : Can't overwrite.\nTo overwrite, use '>|' or Enter [set +o noclobber].\n");
                }
                close(dest);
            }
            else if (strcmp(first_op, ">") == 0 && !set_noclobber)
            {
                dest = open(tokens_list[2][0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (dest > 0)
                {
                    int stdout_cp = dup(1);
                    dup2(dest, 1);
                    run_cmd(token_cnt_list[0], tokens_list[0], false);
                    fflush(stdout);
                    dup2(stdout_cp, 1);
                }
                close(dest);
            }
            else if (strcmp(first_op, ">|") == 0)
            {
                dest = open(tokens_list[2][0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                int stdout_cp = dup(1);
                dup2(dest, 1);
                run_cmd(token_cnt_list[0], tokens_list[0], false);
                fflush(stdout);
                dup2(stdout_cp, 1);
                close(dest);
            }
            else if (strcmp(first_op, ">>") == 0)
            {
                dest = open(tokens_list[2][0], O_WRONLY | O_CREAT | O_APPEND, 0644);
                int stdout_cp = dup(1);
                dup2(dest, 1);
                run_cmd(token_cnt_list[0], tokens_list[0], false);
                fflush(stdout);
                dup2(stdout_cp, 1);
                close(dest);
            }
            else if (strcmp(first_op, "<") == 0)
            {
                dest = open(tokens_list[2][0], O_RDONLY);
                int stdin_cp = dup(0);
                dup2(dest, 0);
                run_cmd(token_cnt_list[0], tokens_list[0], false);
                fflush(stdin);
                dup2(stdin_cp, 0);
                close(dest);
            }
            else if (strcmp(first_op, "|") == 0)
            {
                run_pipe(token_cnt_list, tokens_list, op_num);
            }
        }
        else
        {
            // check all op are pipe or not
            bool op_are_all_pipe = true;
            for (int i=0; i<op_num; i++)
            {
                if (strcmp(tokens_list[2*i+1][0], "|") != 0)
                    op_are_all_pipe = false;
            }

            if (op_are_all_pipe)
            {
                multiple_pipe(token_cnt_list, tokens_list, op_num);
            }
        }
    }
}


void run_pipe(int token_cnt_list[], char* tokens_list[][50], int pipe_num)
{
    int pd[2];
    pid_t pid_pipe;
    pid_t pid;
    pipe(pd);

    if (pipe_num == 1)
    {
        pid_pipe = fork();
        if (pid_pipe == 0)
        {
            pid = fork();
            if (pid == 0)
            {
                dup2(pd[0], 0);
                close(pd[0]);
                close(pd[1]);
                if (token_cnt_list[2] == 1)
                    execlp(tokens_list[2][0], tokens_list[2][0], NULL);
                else
                    execvp(tokens_list[2][0], tokens_list[2]);
            }
            else
            {
                dup2(pd[1], 1);
                close(pd[0]);
                close(pd[1]);
                if (token_cnt_list[0] == 1)
                    execlp(tokens_list[0][0], tokens_list[0][0], NULL);
                else
                    execvp(tokens_list[0][0], tokens_list[0]);
            }
        }
        else
        {
            waitpid(pid_pipe, NULL, 0);
            close(pd[0]);
            close(pd[1]);
        }
    }
}


void multiple_pipe(int token_cnt_list[], char* tokens_list[][50], int pipe_num)
{
    int pid;
    int pip[pipe_num][2];

    // Init
    pipe(pip[0]);
    pid = fork();
    if (pid == 0)
    {
        dup2(pip[0][1], 1);
        close(pip[0][0]);
        close(pip[0][1]);
        if (token_cnt_list[0] == 1)
            execlp(tokens_list[0][0], tokens_list[0][0], NULL);
        else
            execvp(tokens_list[0][0], tokens_list[0]);
    }

    // Inter
    for(int i=1; i<pipe_num; i++)
    {
        pipe(pip[i]);
        pid = fork();
        if (pid == 0)
        {
            dup2(pip[i-1][0], 0);
            dup2(pip[i][1], 1);
            for(int j=0; j<=i; j++)
            {
                close(pip[j][0]);
                close(pip[j][1]);
            }
            if (token_cnt_list[i*2] == 1)
                execlp(tokens_list[i*2][0], tokens_list[i*2][0], NULL);
            else
                execvp(tokens_list[i*2][0], tokens_list[i*2]);     
        }
    }

    // Last
    for(int i=0; i<pipe_num-1; i++)
    {
        close(pip[i][0]);
        close(pip[i][1]);
    }
    pid = fork();
    if (pid == 0)
    {
        dup2(pip[pipe_num-1][0], 0);
        close(pip[pipe_num-1][0]);
        close(pip[pipe_num-1][1]);

        if (token_cnt_list[pipe_num*2] == 1)
            execlp(tokens_list[pipe_num*2][0], tokens_list[pipe_num*2][0], NULL);
        else
            execvp(tokens_list[pipe_num*2][0], tokens_list[pipe_num*2]);
    }
    else
    {
        for(int i=0; i<pipe_num; i++)
        {
            close(pip[i][0]);
            close(pip[i][1]);
        }
        sleep(1);
    }
}
