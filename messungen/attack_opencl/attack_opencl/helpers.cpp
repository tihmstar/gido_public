//
//  helpers.cpp
//  attack_opencl
//
//  Created by tihmstar on 05.12.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#include "helpers.hpp"
#include <time.h>
#include <mutex>

static long kern_avg_runner_cnt = 0;
static double kern_avg_run_time = 0;
std::mutex gLock;

#define NSEC 1e-9

double timespec_to_double(struct timespec *t){
    return ((double)t->tv_sec) + ((double) t->tv_nsec) * NSEC;
}

void double_to_timespec(double dt, struct timespec *t){
    t->tv_sec = (long)dt;
    t->tv_nsec = (long)((dt - t->tv_sec) / NSEC);
}

void get_time(struct timespec *t){
    clock_gettime(CLOCK_MONOTONIC, t);
}

void relaxedWaitForEvent(cl_uint num_events, const cl_event * event_list){
    struct timespec start_time;
    struct timespec t;
    struct timespec end_time;
    double dstart, dend, delta;

    get_time(&start_time);
    
    double_to_timespec(kern_avg_run_time*0.95, &t);
    nanosleep(&t, NULL);
    clWaitForEvents(num_events, event_list);

    get_time(&end_time);

    dstart = timespec_to_double(&start_time);
    dend = timespec_to_double(&end_time);

    delta = dend - dstart;
    gLock.lock();
    kern_avg_run_time += (delta-kern_avg_run_time)/(++kern_avg_runner_cnt);
    printf("kern_avg_run_time=%f\n",kern_avg_run_time);
    gLock.unlock();
}



