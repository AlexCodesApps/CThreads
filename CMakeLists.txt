cmake_minimum_required(VERSION 3.10)
project(CThreads LANGUAGES C)
add_library(CThreads)
target_sources(CThreads PRIVATE threads.c)
