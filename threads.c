#include "threads.h"

#if C_THREADS_PLATFORM == 0

#include <threads.h>

bool thread_start(Thread * thread, ThreadFn fun, void *arg) {
	return thrd_create(thread, fun, arg) == 0;
}

bool thread_detach(Thread * thread) {
	return thrd_detach(*thread) == 0;
}

bool thread_join(Thread * thread, int * status) {
	return thrd_join(*thread, status) == 0;
}

ThreadId thread_id(const Thread *thread) {
	return *thread;
}

ThreadId thread_id_current() {
	return thrd_current();
}

void thread_yield() {
	thrd_yield();
}

bool mutex_init(Mutex * mutex) {
	return mtx_init(mutex, mtx_plain) == 0;
}

bool mutex_lock(Mutex * mutex) {
	return mtx_lock(mutex) == 0;
}

bool mutex_unlock(Mutex * mutex) {
	return mtx_unlock(mutex) == 0;
}

bool mutex_destroy(Mutex * mutex) {
	mtx_destroy(mutex);
	return true;
}

#elif C_THREADS_PLATFORM == 1

#include <pthread.h>
#include <stdlib.h>
#include <stdatomic.h>

typedef struct {
	ThreadFn fun;
	void * arg;
	int status;
	atomic_flag should_free;
} ThreadArgs;

static void * _thread_start(void * arg) {
	ThreadArgs * thread_args = arg;
	void * arg2 = thread_args->arg;
	ThreadFn fun = thread_args->fun;
	int result = fun(arg2);
	thread_args->status = result;
	int should_free = atomic_flag_test_and_set(&thread_args->should_free);
	if (should_free) {
		free(thread_args);
	}
	return NULL;
}

bool thread_start(Thread * thread, ThreadFn fun, void * arg) {
	ThreadArgs * thread_args = malloc(sizeof(ThreadArgs));
	if (!thread_args) {
		return false;
	}
	thread_args->fun = fun;
	thread_args->arg = arg;
	thread_args->should_free = (atomic_flag)ATOMIC_FLAG_INIT;
	int result = pthread_create(&thread->pthread, NULL, _thread_start, thread_args);
	if (result != 0) {
		free(thread_args);
		return false;
	}
	thread->internal = thread_args;
	return true;
}

bool thread_detach(Thread * thread) {
	ThreadArgs * thread_args = thread->internal;
	int should_free = atomic_flag_test_and_set(&thread_args->should_free);
	if (should_free) {
		free(thread_args);
	}
	return pthread_detach(thread->pthread) == 0;
}

bool thread_join(Thread * thread, int * status) {
	ThreadArgs * thread_args = thread->internal;
	if (pthread_join(thread->pthread, NULL) != 0) {
		return false;
	}
	if (status)
		*status = thread_args->status;
	free(thread_args);
	return true;
}

ThreadId thread_id(const Thread * thread) {
	return thread->pthread;
}

ThreadId thread_id_current() {
	return pthread_self();
}

void thread_yield() {
	sched_yield();
}

bool mutex_init(Mutex * mutex) {
	return pthread_mutex_init(mutex, NULL) == 0;
}

bool mutex_lock(Mutex * mutex) {
	return pthread_mutex_lock(mutex) == 0;
}

bool mutex_unlock(Mutex * mutex) {
	return pthread_mutex_unlock(mutex) == 0;
}

bool mutex_destroy(Mutex * mutex) {
	return pthread_mutex_destroy(mutex) == 0;
}

#else

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

// Awful single threaded compatability layer

static int current_id = 0;

bool thread_start(Thread * thread, ThreadFn fun, void * arg) {
	static int id_counter = 0;
	int saved_id = current_id;
	if (id_counter == INT_MAX) {
		fprintf(stderr, "CThreads compatability layer : id counter overflow");
		abort();
	}
	current_id = id_counter++;
	thread->status = fun(arg);
	thread->id = current_id;
	current_id = saved_id;
	return true;
}

bool thread_detach(Thread * thread) {
	return true;
}

bool thread_join(Thread * thread, int * status) {
	if (status)
		*status = thread->status;
	return true;
}

ThreadId thread_id(const Thread * thread) {
	return thread->id;
}

ThreadId thread_id_current() {
	return current_id;
}

// Bc of single threading, this is useless
bool mutex_init(Mutex * mutex) {
	*mutex = 0;
	return true;
}
bool mutex_lock(Mutex * mutex) {
	if (*mutex == 1) {
		fprintf(stderr, "CThreads compatability layer : lock of already locked mutex\n");
		abort();
	}
	return true;
}
bool mutex_unlock(Mutex * mutex) {
	*mutex = 0;
	return true;
}

bool mutex_destroy(Mutex * mutex) {
	return true;
}

#endif
