#include <stdlib.h>

int main() {
#ifdef __POSIX_VERSION
	return system("make donotcall PTHREAD='-lpthread'");
#else
	return system("make donotcall");
#endif
}

