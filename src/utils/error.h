#ifndef VIDEOBOX_ERROR_H
#define VIDEOBOX_ERROR_H

#include <cstdio>

#define check(ret, message) if(ret<0) {fprintf(stderr, "Error: %s\n", message);return -1;}

inline int error(int ret, const char *message) {
    fprintf(stderr, "Error: %s\n", message);
    return ret;
}

#endif //VIDEOBOX_ERROR_H
