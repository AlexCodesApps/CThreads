#include <stdio.h>
#include "../threads.h"

struct State {
	Mutex mutex;
	int  resource;
};

int run(void * arg) {
	struct State * state = arg;
	mutex_lock(&state->mutex);
	++state->resource;
	// relying on thread_id being an integer maybe not portable? no good choice for api anyways
	printf("thread id [%lu] value [%d]\n", thread_id_current(), state->resource);
	mutex_unlock(&state->mutex);
	return 0;
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
		thread_join(threads + i, NULL);
	}
	return 0;
}

