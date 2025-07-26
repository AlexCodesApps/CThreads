#include <stdio.h>
#include "../threads.h"
#include <threads.h>

struct State {
	Mutex mutex;
	int  resource;
};

int run(void * arg) {
	struct State * state = arg;
	mutex_lock(&state->mutex);
	int item = ++state->resource;
	// relying on ThreadId being an integer maybe not portable? no good choice for api anyways
	printf("thread id [%lu] value [%d]\n", (size_t)thread_id_current(), item);
	mutex_unlock(&state->mutex);
	
	return item;
}

int main() {
	struct State state;
	mutex_init(&state.mutex);
	state.resource = 0;

	Thread threads[10];

	for (int i = 0; i < 10; ++i) {
		thread_start(threads + i, run, &state);
	}
	for (int i = 0; i < 10; ++i) {
		thread_detach(threads + i);
	}

	printf("main thread is ending\n");
	thread_exit(255);
}

