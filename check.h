#ifndef CHECK_H
#define CHECK_H

#include <stdio.h> // For fprintf, perror
#include <stdlib.h> // For exit

#define CHECK(x) \
  do { \
    if (!(x)) { \
      fprintf(stderr, "%s:%d: ", __func__, __LINE__); \
      perror(#x); \
      exit(EXIT_FAILURE); \
    } \
  } while (0)


#endif /* __CHECK_H__ */
