#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <copyfile.h>
#include <mach-o/dyld.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc < 3 || (argc % 2) == 0)
        return EXIT_BAD_ARGS;

    int needs_elevation = 0;
    for (int i = 2; i < argc; i += 2) {
        if (!can_write_to(argv[i])) {
            needs_elevation = 1;
            break;
        }
    }

    if (geteuid() != 0 && needs_elevation) {
        int use_sudo = getenv("CI") != NULL || getenv("GITHUB_ACTIONS") != NULL;

        if (use_sudo) {
            char exe_path[4096];
            uint32_t esize = sizeof(exe_path);
            if (_NSGetExecutablePath(exe_path, &esize) < 0)
                return EXIT_ELEV_FAIL;
            char **args = malloc((size_t)(argc + 2) * sizeof(char *));
            if (!args)
                return EXIT_ELEV_FAIL;
            args[0] = "sudo";
            args[1] = exe_path;
            for (int i = 1; i < argc; i++)
                args[i + 1] = argv[i];
            args[argc + 1] = NULL;
            execvp("sudo", args);
            free(args);
        } else {
            char shell_cmd[7168];
            size_t sc_pos = 0;
            for (int i = 1; i < argc; i += 2) {
                const char *src = argv[i];
                const char *dst = argv[i + 1];

                if (sc_pos > 0) {
                    shell_cmd[sc_pos++] = ' ';
                    shell_cmd[sc_pos++] = '&';
                    shell_cmd[sc_pos++] = '&';
                    shell_cmd[sc_pos++] = ' ';
                }

                const char *p = dst + strlen(dst);
                while (p > dst && p[-1] != '/')
                    p--;
                char dirbuf[4096];
                if (p > dst) {
                    size_t dlen = (size_t)(p - dst);
                    if (dlen >= sizeof(dirbuf))
                        return EXIT_COPY_FAIL;
                    memcpy(dirbuf, dst, dlen);
                    dirbuf[dlen] = 0;
                }

                int n;
                if (p > dst) {
                    n = snprintf(shell_cmd + sc_pos,
                                 sizeof(shell_cmd) - sc_pos,
                                 "mkdir -p \"%s\" && cp -p \"%s\" \"%s\"",
                                 dirbuf, src, dst);
                } else {
                    n = snprintf(shell_cmd + sc_pos,
                                 sizeof(shell_cmd) - sc_pos,
                                 "cp -p \"%s\" \"%s\"", src, dst);
                }
                if (n < 0 || (size_t)n >= sizeof(shell_cmd) - sc_pos)
                    return EXIT_ELEV_FAIL;
                sc_pos += (size_t)n;
            }

            char script[8192];
            size_t pos = 0;
            char *prefix = "do shell script \"";
            while (*prefix && pos < sizeof(script) - 1)
                script[pos++] = *prefix++;
            for (size_t i = 0; i < sc_pos && pos < sizeof(script) - 2; i++) {
                if (shell_cmd[i] == '"')
                    script[pos++] = '\\';
                script[pos++] = shell_cmd[i];
            }
            char *suffix = "\" with administrator privileges";
            while (*suffix && pos < sizeof(script) - 1)
                script[pos++] = *suffix++;
            script[pos] = 0;

            char *osargs[] = { "osascript", "-e", script, NULL };
            execvp("osascript", osargs);
        }

        return EXIT_ELEV_FAIL;
    }

    for (int i = 1; i < argc; i += 2) {
        const char *dst = argv[i + 1];

        const char *p = dst + strlen(dst);
        while (p > dst && p[-1] != '/')
            p--;

        if (p > dst) {
            char dir[4096];
            size_t dlen = (size_t)(p - dst);
            if (dlen >= sizeof(dir))
                return EXIT_COPY_FAIL;
            memcpy(dir, dst, dlen);
            dir[dlen] = 0;
            if (mkdir_p(dir, 0755) < 0)
                return EXIT_COPY_FAIL;
        }

        if (copyfile(argv[i], dst, 0, COPYFILE_ALL) < 0)
            return EXIT_COPY_FAIL;
    }

    return EXIT_SUCCESS;
}