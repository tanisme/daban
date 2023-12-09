//
// Created by tanisme on 2023/12/3.
//

#ifndef COMMON_THREADBIND_H
#define COMMON_THREADBIND_H

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <pthread.h>
#endif

#include <thread>

#ifdef WIN32
bool threadbind(HANDLE tid, int cpuidx);
#else
bool threadbind(pthread_t tid, int cpuidx);
#endif

#endif //COMMON_THREADBIND_H
