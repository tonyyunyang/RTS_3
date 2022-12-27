#define _GNU_SOURCE

#include <sys/fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <unistd.h>

void trace_write(const char *fmt, ...);
static int trace_fd = -1;

void* Thread1(void *args)
{
	cpu_set_t cpuset;
	// CPU_ZERO: This macro initializes the CPU set set to be the empty set.
	CPU_ZERO(&cpuset);
	// CPU_SET: This macro adds cpu to the CPU set set.
	CPU_SET(1, &cpuset);
	
	const int set_result = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  	if (set_result != 0) {
    	printf("Error Setting CPU Affinity\n");
  	}

	trace_write("JATW_Thread_1_1 ... Pi = %f", 3.14);

	double testing = 0.0;
	for(int a=0; a<1000000; a++)
	{
		testing = (a*testing)/(1+a);
	}

	trace_write("JATW_Thread_1_2 ... e = %f", 2.71);
}

void* Thread2(void *args)
{
	cpu_set_t cpuset;
	// CPU_ZERO: This macro initializes the CPU set set to be the empty set.
	CPU_ZERO(&cpuset);
	// CPU_SET: This macro adds cpu to the CPU set set.
	CPU_SET(1, &cpuset);
	
	const int set_result = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  	if (set_result != 0) {
    	printf("Error Setting CPU Affinity\n");
  	}

	trace_write("JATW_Thread_2_1... %f", 3.14567381);

	double testing = 0.0;
	for(int a=0; a<1000000; a++)
	{
		testing = (a*testing)/(1+a);
	}

	trace_write("JATW_Thread_2_2");
}

int main(void)
{
	pthread_t th1, th2;
	printf("%s\n", "This is a Hellow World program to test trace_marker"); 
	trace_fd = open("/sys/kernel/debug/tracing/trace_marker", O_WRONLY);
	if(trace_fd < 0) {
		printf("Error Opening trace marker, try running with sudo\n");
	} else{
		printf("Successfully opened trace_marker\n");
	}
	
	trace_write("JATW_1");

	if(pthread_create(&th1, NULL, Thread1, NULL) != 0)
	{
		printf("Error Spawning Thread1");
	}
	else
	{
		printf("Spawned Thread1");	
	}

	if(pthread_create(&th2, NULL, Thread2, NULL) != 0)
	{
		printf("Error Spawning Thread2");
	}
	else
	{
		printf("Spawned Thread2");	
	}

	pthread_join(th1, NULL);
	pthread_join(th2, NULL);

	trace_write("JATW_2");

	if(close(trace_fd) == 0)
	{
		printf("Successfully closed the file\n");
	}
	
	return 0;
}

void trace_write(const char *fmt, ...)
{
        va_list ap;
        char buf[256];
        int n;

        if (trace_fd < 0)
                return;

        va_start(ap, fmt);
        n = vsnprintf(buf, 256, fmt, ap);
        va_end(ap);

        write(trace_fd, buf, n);
        printf("Written Successfully\n");
}
