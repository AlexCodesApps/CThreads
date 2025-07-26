#include <stdlib.h>
#include "threads.h"

int main() {
#if C_THREADS_PLATFORM == C_THREADS_POSIX
	return system("make donotcall PTHREAD=\"-lpthread\"");
#else
	return system("make donotcall");
#endif
}

