cmake_minimum_required(VERSION 3.10)
project(CThreads LANGUAGES C)
add_library(CThreads)
target_sources(CThreads PRIVATE threads.c)
include(CheckIncludeFile)

check_include_file("threads.h" HAVE_THREADS_H)
check_include_file("unistd.h" HAVE_UNISTD_H)
check_include_file("stdatomic.h" HAVE_ATOMIC_H)

target_compile_definitions(CThreads PRIVATE
	C_THREADS_STDC=1
	C_THREADS_POSIX=2
	C_THREADS_WINDOWS=3
	C_THREADS_FALLBACK=4
)

if(HAVE_THREADS_H)
	target_compile_definitions(CThreads PRIVATE C_THREADS_PLATFORM=C_THREADS_STDC)
elseif(HAVE_UNISTD_H AND HAVE_ATOMIC_H)
	target_compile_definitions(CThreads PRIVATE C_THREADS_PLATFORM=C_THREADS_POSIX)
elseif(WIN32)
	target_compile_definitions(CThreads PRIVATE C_THREADS_PLATFORM=C_THREADS_WINDOWS)
else()
	target_compile_definitions(CThreads PRIVATE C_THREADS_PLATFORM=C_THREADS_FALLBACK)
endif()
