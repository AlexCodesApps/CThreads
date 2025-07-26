#ifndef C_THREADS_H
#define C_THREADS_H

#define C_THREADS_STDC 0
#define C_THREADS_POSIX 1
#define C_THREADS_WINDOWS 2
#define C_THREADS_FALLBACK 3

#ifdef _MSC_VER
#define C_THREADS_NORETURN __declspec(noreturn)
#elif defined(__GNUC__)
#define C_THREADS_NORETURN __attribute__((noreturn))
#elif __cplusplus >= 201103L || __STDC_VERSION__ >= 202000
#define C_THREADS_NORETURN [[noreturn]]
#elif __STDC_VERSION__ == 201112L
#define C_THREADS_NORETURN _Noreturn
#else
#define C_THREADS_NORETURN
#endif

#ifdef __cplusplus
// #include <thread>
// #include <type_traits>
// #include <mutex>
extern "C" {
#endif

#if !defined(__STDC_NO_THREADS__) && __STDC_VERSION__ >= 201112L
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
	char buffer[sizeof(std::thread::id)]; // is this ok? I don't speak standardese
} ThreadId;

typedef struct {
	std::thread * thread;
} Thread;

typedef struct {
	std::mutex * mutex;
} Mutex;
#elif _WIN32
#define C_THREADS_PLATFORM C_THREADS_WINDOWS
#include <Windows.h>

typedef struct {
	HANDLE handle;
	void * internal;
} Thread;
typedef DWORD ThreadId;
typedef HANDLE Mutex;

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
ThreadId thread_id_current(void);
bool thread_ids_equal(ThreadId a, ThreadId b);
void thread_yield(void);
C_THREADS_NORETURN void thread_exit(int status);

bool mutex_init(Mutex * mutex);
bool mutex_lock(Mutex * mutex);
bool mutex_unlock(Mutex * mutex);
bool mutex_destroy(Mutex * mutex);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // C_THREADS_H
