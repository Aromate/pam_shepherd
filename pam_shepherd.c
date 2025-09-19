/*
 * pam_shepherd - Minimal PAM module for GNU Shepherd
 *
 * Fire and forget: Start shepherd if not running, don't care about cleanup.
 *
 * SPDX-License-Identifier: ISC
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <security/pam_modules.h>
#include <security/pam_ext.h>
#include <syslog.h>

#define MODULE_NAME "pam_shepherd"

/* Check if shepherd socket exists */
static int socket_exists(uid_t uid)
{
    char path[PATH_MAX];
    struct stat st;

    snprintf(path, sizeof(path), "/run/user/%d/shepherd/socket", uid);
    return (stat(path, &st) == 0);
}

/* Ensure runtime directory exists */
static void ensure_runtime_dir(uid_t uid)
{
    char dir[PATH_MAX];

    snprintf(dir, sizeof(dir), "/run/user/%d", uid);
    mkdir(dir, 0700); /* Ignore errors - systemd-logind usually handles this */
}

/* Start shepherd using double-fork with proper waitpid */
static void start_shepherd(pam_handle_t *pamh, uid_t uid, gid_t gid, const char *home, const char *user)
{
    pid_t pid = fork();

    if (pid < 0) {
        pam_syslog(pamh, LOG_ERR, "fork failed for %s: %m", user);
        return;
    }

    if (pid > 0) {
        /* Parent waits for intermediate child to prevent zombie */
        waitpid(pid, NULL, 0);
        return;
    }

    /* First child: fork again and exit */
    pid = fork();

    if (pid < 0) {
        _exit(127);
    }

    if (pid > 0) {
        /* Intermediate child exits immediately */
        _exit(0);
    }

    /* Grandchild: cannot acquire controlling terminal */
    setsid();

    /* Redirect I/O to /dev/null */
    int fd = open("/dev/null", O_RDWR);
    if (fd >= 0) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > 2) close(fd);
    }

    /* Drop privileges - fail fast */
    if (setgroups(0, NULL) < 0) _exit(1);
    if (setgid(gid) < 0) _exit(1);
    if (setuid(uid) < 0) _exit(1);

    /* Set environment variables */
    char runtime[PATH_MAX];
    snprintf(runtime, sizeof(runtime), "/run/user/%d", uid);

    setenv("XDG_RUNTIME_DIR", runtime, 1);
    setenv("HOME", home, 1);
    setenv("USER", user, 1);
    setenv("PATH", "/usr/local/bin:/usr/bin:/bin", 1);
    setenv("LANG", "C.UTF-8", 1);

    execlp("shepherd", "shepherd", NULL);
    _exit(127);
}

PAM_EXTERN int
pam_sm_open_session(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    const char *user;
    struct passwd *pwd;

    (void)flags;
    (void)argc;
    (void)argv;

    if (pam_get_user(pamh, &user, NULL) != PAM_SUCCESS) {
        return PAM_SUCCESS;
    }

    pwd = getpwnam(user);
    if (!pwd || pwd->pw_uid == 0) {
        return PAM_SUCCESS; /* Skip root */
    }

    /* Ensure runtime directory exists */
    ensure_runtime_dir(pwd->pw_uid);

    /* Start shepherd if socket doesn't exist */
    if (!socket_exists(pwd->pw_uid)) {
        start_shepherd(pamh, pwd->pw_uid, pwd->pw_gid, pwd->pw_dir, user);
        pam_syslog(pamh, LOG_INFO, "Starting shepherd for %s", user);
    }

    return PAM_SUCCESS;
}

PAM_EXTERN int
pam_sm_close_session(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    /* Don't care about cleanup - shepherd keeps running */
    (void)pamh;
    (void)flags;
    (void)argc;
    (void)argv;

    return PAM_SUCCESS;
}

/* Required PAM functions */
PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    (void)pamh; (void)flags; (void)argc; (void)argv;
    return PAM_IGNORE;
}

PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    (void)pamh; (void)flags; (void)argc; (void)argv;
    return PAM_IGNORE;
}

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    (void)pamh; (void)flags; (void)argc; (void)argv;
    return PAM_IGNORE;
}

PAM_EXTERN int pam_sm_chauthtok(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    (void)pamh; (void)flags; (void)argc; (void)argv;
    return PAM_IGNORE;
}