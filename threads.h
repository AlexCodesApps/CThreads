#ifndef C_THREADS_H
#define C_THREADS_H

#ifdef __cplusplus
// #include <thread>
// #include <type_traits>
// #include <mutex>
extern "C" {
#endif

#if !defined(__STDC_NO_THREADS__)
#define C_THREADS_PLATFORM 0

#include <threads.h>

typedef thrd_t Thread;
typedef thrd_t ThreadId;
typedef mtx_t Mutex;

#elif (defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))) && !defined(__STDC_NO_ATOMIC__)
#define C_THREADS_PLATFORM 1

#include <pthread.h>

typedef struct {
	pthread_t pthread;
	void * internal;
} Thread;
typedef pthread_t ThreadId;
typedef pthread_mutex_t Mutex;

#elif defined(__cplusplus) && 0 // C++ fallback in the future?

static_assert(std::is_trivially_copyable<std::thread::id>::value);
static_assert(std::is_trivially_destructible<std::thread::id>::value);

typedef struct {
	char buffer[sizeof(std::thread::id)]; // is this ok? I don't speak standardese
} ThreadId;

typedef struct {
	std::thread * thread;
} Thread;

typedef struct {
	std::mutex * mutex;
} Mutex;

#else
#define C_THREADS_PLATFORM 2

typedef struct {
	int status;
	int id;
} Thread;
typedef int ThreadId;
typedef int Mutex;
#endif
#include <stdbool.h>

typedef int(*ThreadFn)(void * arg);

bool thread_start(Thread * thread, ThreadFn fun, void * arg);
bool thread_detach(Thread * thread);
bool thread_join(Thread * thread, int * status);
ThreadId thread_id(const Thread * thread);
ThreadId thread_id_current();
bool thread_ids_equal(ThreadId a, ThreadId b);
void thread_yield();
void thread_exit(int status);

bool mutex_init(Mutex * mutex);
bool mutex_lock(Mutex * mutex);
bool mutex_unlock(Mutex * mutex);
bool mutex_destroy(Mutex * mutex);

#ifdef __cplusplus
}
#endif

#endif
