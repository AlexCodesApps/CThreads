#include "threads.h"

#if C_THREADS_PLATFORM == C_THREADS_STDC

#include <stdbool.h>
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

ThreadId thread_id_current(void) {
	return thrd_current();
}

bool thread_ids_equal(ThreadId a, ThreadId b) {
	return thrd_equal(a, b);
}

void thread_yield(void) {
	thrd_yield();
}

C_THREADS_NORETURN void thread_exit(int status) {
	thrd_exit(status);
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

#elif C_THREADS_PLATFORM == C_THREADS_POSIX

#include <pthread.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdint.h>

typedef struct {
	ThreadFn fun;
	void * arg;
	int status;
	atomic_flag should_free;
} ThreadArgs;

static atomic_size_t thread_count = 1;

static pthread_key_t exit_handler_key;
static pthread_once_t exit_handler_key_init_guard;

typedef struct {
	int status;
	jmp_buf jmp;
} ExitHandler;

static void * _thread_start(void * arg) {
	ThreadArgs * thread_args = arg;
	ExitHandler exit_handler;
	size_t previous;
	int result, should_free;
	if (setjmp(exit_handler.jmp) != 0) {
		thread_args->status = exit_handler.status;
		goto cleanup;
	}
	if (pthread_setspecific(exit_handler_key, &exit_handler) != 0) {
		abort();
	}
	previous = atomic_fetch_add_explicit(&thread_count, 1, memory_order_relaxed);
	if (previous == SIZE_MAX) {
		fprintf(stderr, "CThreads pthreads backend : thread counter overflow\n");
		abort();
	}

	result = thread_args->fun(thread_args->arg);

	thread_args->status = result;
cleanup:;
	should_free =
		atomic_flag_test_and_set_explicit(
			&thread_args->should_free,
				memory_order_acq_rel);
	if (should_free) {
		free(thread_args);
	}
	if (atomic_fetch_sub(&thread_count, 1) == 1) {
		exit(result);
	}
	return NULL;
}

static void init_exit_handler_key(void) {
	if (pthread_key_create(&exit_handler_key, NULL) != 0) {
		fprintf(stderr, "CThreads pthreads backend : unable to "
						"allocate thread local exit handler key");
		abort();
	}
	pthread_setspecific(exit_handler_key, NULL);
}

bool thread_start(Thread * thread, ThreadFn fun, void * arg) {
	ThreadArgs * thread_args;
	int result;
	pthread_once(&exit_handler_key_init_guard, init_exit_handler_key);
	thread_args = malloc(sizeof(ThreadArgs));
	if (!thread_args) {
		return false;
	}
	thread_args->fun = fun;
	thread_args->arg = arg;
	atomic_flag_clear_explicit(&thread_args->should_free, memory_order_relaxed);
	result =
		pthread_create(&thread->pthread, NULL,
						_thread_start, thread_args);
	if (result != 0) {
		free(thread_args);
		return false;
	}
	thread->internal = thread_args;
	return true;
}

bool thread_detach(Thread * thread) {
	ThreadArgs * thread_args = thread->internal;
	int should_free =
		atomic_flag_test_and_set_explicit(
			&thread_args->should_free,
				memory_order_acq_rel);
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

ThreadId thread_id_current(void) {
	return pthread_self();
}

bool thread_ids_equal(ThreadId a, ThreadId b) {
	return pthread_equal(a, b) != 0;
}

void thread_yield(void) {
	sched_yield();
}

C_THREADS_NORETURN void thread_exit(int status) {
	ExitHandler * exit_handler = pthread_getspecific(exit_handler_key);
	if (exit_handler == NULL) { // main thread
		if (atomic_fetch_sub(&thread_count, 1) == 1) {
			exit(status);
		}
		pthread_exit(NULL);
	}
	exit_handler->status = status;
	longjmp(exit_handler->jmp, 1);
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

#elif C_THREADS_PLATFORM == C_THREADS_WINDOWS

#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>

typedef struct {
	ThreadFn fun;
	void * arg;
	int status;
	LONG should_free;
} ThreadArgs;

DWORD _thread_start(void * arg) {
	ThreadArgs * thread_args = arg;

	void * arg2 = thread_args->arg;
	ThreadFn fun = thread_args->fun;

	int result = fun(arg2);

	int should_free;

	thread_args->status = result;

	should_free =
		InterlockedBitTestAndSetNoFence(
			&thread_args->status,
			0);
	if (should_free) {
		free(thread_args);
	}
	return result;
}

bool thread_start(Thread * thread, ThreadFn fun, void * arg) {
	HANDLE new_thread;
	ThreadArgs * thread_args = malloc(sizeof(ThreadArgs));
	if (!thread_args) {
		return false;
	}
	thread_args->fun = fun;
	thread_args->arg = arg;
	thread_args->should_free = 0;
	new_thread = CreateThread(NULL, 0, fun, thread_args, 0, 0);
	if (!new_thread) {
		free(thread_args);
		return false;
	}
	thread->handle = new_thread;
	thread->internal = thread_args;
	return true;
}

bool thread_detach(Thread * thread) {
	ThreadArgs * thread_args = thread->internal;
	int should_free =
		InterlockedBitTestAndSetNoFence(
			&thread_args->should_free, 0);
	if (should_free) {
		free(thread_args);
	}
	return CloseHandle(thread->handle);
}

bool thread_join(Thread * thread, int * status) {
	ThreadArgs * thread_args = thread->internal;
	if (WaitForSingleObject(thread->handle, INFINITE) == WAIT_FAILED) {
		return false;
	}
	CloseHandle(thread->handle);
	if (status)
		*status = thread_args->status;
	free(thread_args);

	return true;
}

ThreadId thread_id(const Thread * thread) {
	return thread->handle;
}

ThreadId thread_id_current(void) {
	return GetCurrentThreadId();
}

bool thread_ids_equal(ThreadId a, ThreadId b) {
	return a == b;
}

void thread_yield(void) {
	SwitchToThread();
}

void thread_exit(int status) {
	ExitThread(status);
}

bool mutex_init(Mutex * mutex) {
	HANDLE new_mutex = CreateMutexA(NULL, false, NULL);
	if (!new_mutex) {
		return false;
	}
	*mutex = new_mutex;
	return true;
}

bool mutex_lock(Mutex * mutex) {
	return WaitForSingleObject(*mutex, INFINITE) != WAIT_FAILED;
}

bool mutex_unlock(Mutex * mutex) {
	return ReleaseMutex(*mutex);
}

bool mutex_destroy(Mutex * mutex) {
	return CloseHandle(*mutex);
}

#else /* C_THREADS_PLATFORM == C_THREADS_FALLBACK */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <setjmp.h>

/* Awful single threaded compatibility layer */

static int current_id = 0;
static struct ExitHandler {
	int status;
	jmp_buf buf;
} exit_handler;

bool thread_start(Thread * thread, ThreadFn fun, void * arg) {
	static int id_counter = 0;
	int saved_id = current_id;
	struct ExitHandler saved_handler = exit_handler;
	if (id_counter == INT_MAX) {
		fprintf(stderr, "CThreads compatibility layer : id counter overflow\n");
		abort();
	}

	current_id = ++id_counter;

	if (setjmp(exit_handler.buf) != 0) {
		thread->status = exit_handler.status;
		goto cleanup;
	}

	thread->status = fun(arg);

cleanup:
	exit_handler = saved_handler;
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

ThreadId thread_id_current(void) {
	return current_id;
}

bool thread_ids_equal(ThreadId a, ThreadId b) {
	return a == b;
}

void thread_yield(void) {}

C_THREADS_NORETURN void thread_exit(int status) {
	if (thread_id_current() == 0) {
		exit(status);
	}
	longjmp(exit_handler.buf, 1);
}

/* Bc of single threading, this is useless */
bool mutex_init(Mutex * mutex) {
	*mutex = 0;
	return true;
}

bool mutex_lock(Mutex * mutex) {
	if (*mutex == 1) {
		fprintf(stderr, "CThreads compatibility layer : lock of already locked mutex\n");
		abort();
	}
	*mutex = 1;
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
