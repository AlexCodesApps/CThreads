#ifndef C_THREADS_H
#define C_THREADS_H

#define C_THREADS_STDC 0
#define C_THREADS_POSIX 1
#define C_THREADS_FALLBACK 2

#ifdef __cplusplus
// #include <thread>
// #include <type_traits>
// #include <mutex>
extern "C" {
#if __cplusplus < 201103L
#error unsupported C++ standard
#else
#define C_THREADS_NO_RETURN [[noreturn]]
#define C_THREADS_THREADLOCAL thread_local
#endif // __cplusplus < 201103L
#elif !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 201112L)
#error unsupported C standard
#elif __STDC_VERSION__ >= 202311L // C23
#define C_THREADS_NO_RETURN [[noreturn]]
#define C_THREADS_THREADLOCAL thread_local
#else // C11
#define C_THREADS_NO_RETURN _Noreturn
#define C_THREADS_THREADLOCAL _Thread_local
#endif // __cplusplus

#if !defined(__STDC_NO_THREADS__)
#define C_THREADS_PLATFORM C_THREADS_STDC

#include <threads.h>

typedef thrd_t Thread;
typedef thrd_t ThreadId;
typedef mtx_t Mutex;

#elif (defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))) && !defined(__STDC_NO_ATOMIC__)
#define C_THREADS_PLATFORM C_THREADS_POSIX

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
#define C_THREADS_PLATFORM C_THREADS_FALLBACK

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
C_THREADS_NO_RETURN void thread_exit(int status);

bool mutex_init(Mutex * mutex);
bool mutex_lock(Mutex * mutex);
bool mutex_unlock(Mutex * mutex);
bool mutex_destroy(Mutex * mutex);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // C_THREADS_H
