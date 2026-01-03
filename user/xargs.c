#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define MAXLINE 512
#define MAXARGS 64

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(2, "Usage: xargs <command> [args...]\n");
        exit(1);
    }

    char buf[MAXLINE];
    int n;

    while ((n = read(0, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';

        char *p = buf;
        while (*p) {
            char *line_start = p;
            while (*p != '\n' && *p != '\0') p++;
            if (*p == '\n') *p++ = '\0'; 

            if (*line_start == '\0') continue;

            char *args[MAXARGS];
            int arg_idx = 0;
            for (int i = 1; i < argc; i++) {
                if (arg_idx >= MAXARGS - 2) break; // 留空间给新参数和 NULL
                args[arg_idx++] = argv[i];
            }

            // 添加当前行作为最后一个参数
            args[arg_idx++] = line_start;
            args[arg_idx] = 0;  // NULL terminate

            int pid = fork();
            if (pid == 0) {
                // 子进程
                exec(argv[1], args);
                fprintf(2, "xargs: exec failed\n");
                exit(1);
            } else if (pid > 0) {
                // 父进程等待子进程结束
                wait(0);
            } else {
                fprintf(2, "xargs: fork failed\n");
                exit(1);
            }
        }
    }

    exit(0);
}
