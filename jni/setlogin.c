#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#include <unistd.h>
#include <stdlib.h>
#include "setlogin.h"

static void pwtoid(const char *tok, uid_t *uid, gid_t *gid)
{
    struct passwd *pw;
    pw = getpwnam(tok);
    if (pw) {
        if (uid) *uid = pw->pw_uid;
        if (gid) *gid = pw->pw_gid;
    } else {
        uid_t tmpid = atoi(tok);
        if (uid) *uid = tmpid;
        if (gid) *gid = tmpid;
    }
}

static void extract_uidgids(const char *uidgids, uid_t *uid, gid_t *gid, gid_t *gids,
                     int *gids_count)
{
    char *clobberablegids;
    char *nexttok;
    char *tok;
    int gids_found;

    if (!uidgids || !*uidgids) {
        *gid = *uid = 0;
        *gids_count = 0;
        return;
    }
    clobberablegids = strdup(uidgids);
    // strcpy(clobberablegids, uidgids);
    nexttok = clobberablegids;
    tok = strsep(&nexttok, ",");
    pwtoid(tok, uid, gid);
    tok = strsep(&nexttok, ",");
    if (!tok) {
        /* gid is already set above */
        *gids_count = 0;
        free(clobberablegids);
        return;
    }
    pwtoid(tok, NULL, gid);
    gids_found = 0;
    while ((gids_found < *gids_count) && (tok = strsep(&nexttok, ","))) {
        pwtoid(tok, NULL, gids);
        gids_found++;
        gids++;
    }
    if (nexttok && gids_found == *gids_count) {
        fprintf(stderr, "too many group ids\n");
    }
    *gids_count = gids_found;
    free(clobberablegids);
}

int setlogin(const char *login) {
    uid_t uid;
    gid_t gid, gids[10];
    int gids_count = sizeof(gids) / sizeof(gids[0]);
    extract_uidgids(login, &uid, &gid, gids, &gids_count);
    if (gids_count) {
        if (setgroups(gids_count, gids)) {
            perror("setgroups");
            return -errno;
        }
    }
    if (setgid(gid)) {
        perror("setgid");
        return -errno;
    }
    if (setuid(uid)) {
        perror("setuid");
        return -errno;
    }
    return 0;
}
