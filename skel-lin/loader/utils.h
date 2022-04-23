#ifndef UTILS_H
#define UTILS_H 1

#include <stdio.h>
#include <stdlib.h>

/* Useful macro for handling error codes */
#define DIE(assertion, call_description)					\
	do {									\
		if (assertion) {						\
			fprintf(stderr, "(%s, %s, %d): ",			\
					__FILE__, __FUNCTION__, __LINE__);	\
			perror(call_description);			\
			exit(EXIT_FAILURE);					\
		}								\
	} while (0)

#endif /* UTILS_H */