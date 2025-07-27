#include <stdio.h>
#include <stdlib.h>
#include "../threads.h"

struct State {
	Mutex mutex;
	int  resource;
};

int run(void * arg) {
	struct State * state = arg;
	mutex_lock(&state->mutex);
	int item = ++state->resource;
	printf("value [%d]\n", item);
	mutex_unlock(&state->mutex);
	
	printf("ending [%d]\n", item);
	return item;
}

int main() {
	struct State state;
	mutex_init(&state.mutex);
	state.resource = 0;

	Thread threads[10];

	for (int i = 0; i < 10; ++i) {
		if (!thread_start(threads + i, run, &state))
			abort();
	}
	for (int i = 0; i < 10; ++i) {
		thread_detach(threads + i);
	}

	printf("main thread is ending\n");
	thread_exit(255);
}

