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
        char exe_path[4096];
        uint32_t esize = sizeof(exe_path);
        if (_NSGetExecutablePath(exe_path, &esize) < 0)
            return EXIT_ELEV_FAIL;

        int use_sudo = getenv("CI") != NULL || getenv("GITHUB_ACTIONS") != NULL;

        if (use_sudo) {
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
            size_t total = 0;
            for (int i = 1; i < argc; i++)
                total += strlen(argv[i]) + 3;
            char *cmdline = malloc(total + 1);
            if (!cmdline)
                return EXIT_ELEV_FAIL;
            cmdline[0] = 0;
            for (int i = 1; i < argc; i++) {
                if (i > 1) strcat(cmdline, " ");
                strcat(cmdline, "\"");
                strcat(cmdline, argv[i]);
                strcat(cmdline, "\"");
            }

            char script[8192];
            snprintf(script, sizeof(script),
                     "do shell script \"\\\"%s\\\" %s\" with administrator privileges",
                     exe_path, cmdline);
            free(cmdline);

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
            mkdir_p(dir, 0755);
        }

        if (copyfile(argv[i], dst, 0, COPYFILE_ALL) < 0)
            return EXIT_COPY_FAIL;
    }

    return EXIT_SUCCESS;
}