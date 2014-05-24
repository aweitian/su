#ifdef ANDROID
#define LOGPATH "/sdcard/su.log"
#else
#define LOGPATH "su.log"
#endif
void logargv(char *parent, int allowed, int argc, char **argv);
void logstdin(ssize_t (*canprocess)(void), ssize_t (*process)(void *data, const void *buf, size_t nbyte), void *data);
