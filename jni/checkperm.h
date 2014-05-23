int istrust();
int hasinput();
int checkperm(int argc, char **argv);
void process(ssize_t (*w)(int fildes, const void *buf, size_t nbyte), int fildes);
