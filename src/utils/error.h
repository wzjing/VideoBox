#ifndef VIDEOBOX_ERROR_H
#define VIDEOBOX_ERROR_H

#include <cstdio>

#define check(ret, message) if(ret<0) {fprintf(stderr, "Error: %s\n", message);return -1;}
#define error(ret, message) error_msg(ret, message, __FILE__, __LINE__)

inline int error_msg(int ret, const char *message, const char* filename, int line) {
    fprintf(stderr, "Error: %s\n", message);
    return ret;
}

#endif //VIDEOBOX_ERROR_H
