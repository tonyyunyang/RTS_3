/* Created by: Muhammad Junaid Aslam
*  Teaching Assistant, Real-Time Systems Course IN4343, 2020
*  PhD Candidate Technical University of Delft, Netherlands.
*/

// Header File inclusion
#define _GNU_SOURCE
#include <sys/fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include <sched.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <linux/unistd.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>

// Global Defines
#define TRUE 1
#define FALSE 0

#define NUM_THREADS 6
#define PERIOD 5 // define how many times do you want the tasks to be called
#define HIGHEST_PRIORITY	99

#define handle_error_en(en, msg) \
               do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define POL_TO_STR(x) \
       x==SCHED_FIFO?"FIFO":x==SCHED_RR?"RR":x==SCHED_OTHER?"OTHER":"INVALID"

#define DEFAULT_BUSY_WAIT_TIME 5000 // 5 Milliseconds
// #define DEFAULT_BUSY_WAIT_TIME 150000 // default: 5 Milliseconds
#define DEFAULT_RR_LOOP_TIME 1000 // 1 Millisecond
#define MICRO_SECOND_MULTIPLIER 1000000

// Global data and structures
static int trace_fd = -1;
static int inputpolicy = 0;
long long* responseTimeArray;
long long* deadlineArray;
long long* expectReleaseTime;
long long* actualReleaseTime;
long long* diffReleaseTime;
int position_count = 0;



struct thread_args {
	unsigned int thread_period;
	unsigned int thread_priority;
	unsigned int thread_policy;
	unsigned int thread_affinity; 
	unsigned int thread_number;
	struct timespec thread_start_time;
	struct timespec thread_end_time;
	unsigned long long thread_end_timestamp;
	double thread_exectime;
	long long thread_deadline;
	long long thread_response_time;

	struct timespec wake_time; // added to calculate the release time of each thread
};

static struct thread_args results[NUM_THREADS];
typedef void* (*thread_func_prototype)(void*);

/*<======== Do not change anything in this function =========>*/
static void timespec_add_us(struct timespec *t, long us)
{
	// add microseconds to timespecs nanosecond counter
	t->tv_nsec += us*1000;
	// if wrapping nanosecond counter, increment second counter
	if (t->tv_nsec > 1000000000)
	{
		t->tv_nsec = t->tv_nsec - 1000000000;
		t->tv_sec += 1;
	}
}

/*<======== Do not change anything in this function =========>*/
static inline int timespec_cmp(struct timespec *now, struct timespec *next)
{
    int diff =  now->tv_sec - next->tv_sec;
    return diff ? diff : now->tv_nsec - next->tv_nsec;
}

/*<======== Do not change anything in this function =========>*/
static unsigned long long getTimeStampMicroSeconds(void)
{
	struct timeval currentTime;
	gettimeofday(&currentTime, NULL);
	return currentTime.tv_sec * 1000000 + currentTime.tv_usec;
}

/*<======== Do not change anything in this function =========>*/
static long clock_diff(struct timespec start, struct timespec end)
{
	/* It returns difference in time in nano-seconds */
    struct timespec temp;

    if ((end.tv_nsec-start.tv_nsec)<0)
    {
            temp.tv_sec = end.tv_sec-start.tv_sec-1;
            temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    }
    else 
    {
            temp.tv_sec = end.tv_sec-start.tv_sec;
            temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp.tv_nsec;
}

/*<======== Do not change anything in this function =========>*/
static int set_attributes(int *priorities, int num_thr, int *Affinities, int *Policies, pthread_attr_t *attributes)
{
	cpu_set_t cpuset;

	for(int i=0; i<num_thr; i++)
	{
		if((pthread_attr_init(&attributes[i])!=0)) {
			printf("%d - %s %d\n", __LINE__, "Error Initializing Attributes for Thread:", i+1);
			return FALSE;
		}

		struct sched_param param;

		param.sched_priority = priorities[i];

		CPU_ZERO(&cpuset);
		// CPU_SET: This macro adds cpu to the CPU set set.
		CPU_SET(Affinities[i], &cpuset);

		if((pthread_attr_setschedpolicy(&attributes[i], Policies[i])!=0)) 
		{
			printf("%s %d\n", "Error Setting Scheduling Policy for Thread:", i);
			return FALSE;
		}

		if((pthread_attr_setschedparam(&attributes[i], &param) != 0)) 
		{
			printf("%s %d\n", "Error Setting Scheduling Parameters for Thread:", i);
			return FALSE;
		} 

		if((pthread_attr_setaffinity_np(&attributes[i], sizeof(cpu_set_t), &cpuset)!=0))
		{
			printf("%s %d\n", "Error Setting CPU Affinities for Thread:", i);
			return FALSE;
		}

		if((pthread_attr_setinheritsched(&attributes[i], PTHREAD_EXPLICIT_SCHED)!=0))
		{
			printf("%s %d\n", "Error Setting CPU Attributes for Thread:", i);
			return FALSE;
		}	
	}

	return TRUE;
}

/*<======== Do not change anything in this function =========>*/
static void trace_write(const char *fmt, ...)
{
        va_list ap;
        char buf[256];
        int n;

        if (trace_fd < 0){return;}

        va_start(ap, fmt);
        n = vsnprintf(buf, 256, fmt, ap);
        va_end(ap);

        write(trace_fd, buf, n);
}

/*<======== Do not change anything in this function =========>*/
static void workload(int tid)
{

	struct timespec lvTimeVal;
	if(clock_gettime(CLOCK_THREAD_CPUTIME_ID, &lvTimeVal) != 0){
		fprintf(stderr, "%s\n", "Error Fetching Clock Start Time."); return;
	}
	struct timespec lvTimer = lvTimeVal, lvTimer2 = lvTimeVal;
	unsigned int counter = 1;


	while(1)
	{
		if(clock_gettime(CLOCK_THREAD_CPUTIME_ID, &lvTimeVal) != 0){
			fprintf(stderr, "%s\n", "Error Fetching Clock Start Time."); return;
		}

		if  (
				(
					(lvTimeVal.tv_sec*MICRO_SECOND_MULTIPLIER+lvTimeVal.tv_nsec/1000) - 
					(lvTimer.tv_sec*MICRO_SECOND_MULTIPLIER+lvTimer.tv_nsec/1000)
				) 
					>= DEFAULT_BUSY_WAIT_TIME
			) {break;}

		if  (
				(inputpolicy == SCHED_RR) && 
				(
					(
						(lvTimeVal.tv_sec*MICRO_SECOND_MULTIPLIER+lvTimeVal.tv_nsec/1000) - 
						(lvTimer2.tv_sec*MICRO_SECOND_MULTIPLIER+lvTimer2.tv_nsec/1000)
					)
					 	>= DEFAULT_RR_LOOP_TIME
				)
			) 
		{
			lvTimer2.tv_sec = lvTimeVal.tv_sec;
			lvTimer2.tv_nsec = lvTimeVal.tv_nsec;
			trace_write("RTS_Thread_%d Loop ... %d", tid, counter++);
		}
	}
}

/*<======== This is where you have to work =========>*/
static void* Thread(void *inArgs)
{
	long long thread_response_time = 0;
	int tasks_count = 0;
	int miss_flag;
	long long miss_deadline_time = 0;

	/* <==================== ADD CODE BELOW =======================>*/
	/* Follow the instruction manual for creating a periodic task here
	 * The given code snippet of this thread is one shot, which means that
	 * it will execute once and exit. However, in real-time systems, a thread
	 * is normally periodic. You can also check the deadline miss of a thread
	 * following the same tutorial. Use the given clock_gettime() examples to
	 * find out the response_time of the threads. For calculating the next
	 * period of the thread use the period value given in thread_args struct.
	 */
	
	struct thread_args *args = (struct thread_args*)inArgs; /*This fetches the thread control block (thread_args)
	passed to the thread at the time of creation*/
	
	int tid = args->thread_number; /* tid is just the number of the executing thread; Note that, 
	pthread_t specified thread id is no the same as this thread id as this depicts only the sequence number of this thread.*/

	while (tasks_count < PERIOD) {

		// struct timespec tp;
		// int ret;
		// pid_t pid;
		// pid = getpid();
		// ret = sched_rr_get_interval(pid, &tp);
		// printf("Current pid: %d Round-robin time interval: %ld.%09ld seconds\n", pid, tp.tv_sec, tp.tv_nsec);

		miss_flag = 0;
		clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &args->wake_time, NULL);

		actualReleaseTime[position_count] = args->wake_time.tv_sec*1000000000 + args->wake_time.tv_nsec;
		expectReleaseTime[position_count] = args->wake_time.tv_sec*1000000000 + args->wake_time.tv_nsec;

		trace_write("RTS_Thread_%d Policy:%s Priority:%d Period: %d\n", /*This is an example trace message which appears at the start of the thread in KernelShark */
		args->thread_number, POL_TO_STR(args->thread_policy), args->thread_priority, args->thread_period*1000);

		clock_gettime(CLOCK_REALTIME, &results[tid].thread_start_time); // This fetches the timespec structure through which can get current time.
		workload(args->thread_number); // This produces a busy wait loop of ~5+/-100us milliseconds
		/* In order to change the execution time (busy wait loop) of this thread
		*  from ~5+/-100us milliseconds to XX milliseconds, you have to change the value of
		*  DEFAULT_BUSY_WAIT_TIME macro at the top of this file. 
		*/
		clock_gettime(CLOCK_REALTIME, &results[tid].thread_end_time);

		/* Following sequence of commented instructions should be filled at the end of each periodic iteration*/
		results[tid].thread_deadline 		= args->wake_time.tv_sec * 1000000000 + args->wake_time.tv_nsec + args->thread_period * 1000;
		results[tid].thread_response_time 	= clock_diff(args->wake_time, results[tid].thread_end_time);

		/* Do not change the below sequence of instructions.*/
		results[tid].thread_number 			= args->thread_number;
		results[tid].thread_policy 			= args->thread_policy; 
		results[tid].thread_affinity 		= sched_getcpu();
		results[tid].thread_priority 		= args->thread_priority;
		results[tid].thread_end_timestamp 	= getTimeStampMicroSeconds();

		// incrementing first is necessary, because you want to compare it with the next release time
		timespec_add_us(&args->wake_time, (args->thread_period));

		miss_deadline_time = timespec_cmp(&results[tid].thread_end_time, &args->wake_time);

		if (miss_deadline_time > 0) {
			miss_flag = 1;
		}

		trace_write("RTS_Thread_%d Terminated ... ResponseTime:%lld Deadline:%lld Miss: %d Miss time: %lld\n", tid, results[tid].thread_response_time, results[tid].thread_deadline, miss_flag, miss_deadline_time);

		// if ((args->thread_policy == SCHED_RR) || (args->thread_policy == SCHED_OTHER)){
		// 	if (miss_flag == 1) {
		// 	break;
		// 	}
		// }
		
		tasks_count++;

		// fill the global array for dumping into file
		responseTimeArray[position_count] = results[tid].thread_response_time;
		deadlineArray[position_count] = results[tid].thread_deadline;
		position_count++;
	}

	/* <==================== ADD CODE ABOVE =======================>*/
}

/*<======== Do not change anything in this function =========>*/
static void help(void) 
{	
	printf("Error... Provide One of the Following Scheduling Policies\n");
	printf("1 - FIFO\n");
	printf("2 - RR\n");
	printf("4 - OTHER\n");
}

/*<======== Do not change anything in this function =========>*/
static int comparator(const void* p, const void* q) 
{ 
   if (((struct thread_args*)p)->thread_end_timestamp < ((struct thread_args*)q)->thread_end_timestamp) {return -1;}
   else if (((struct thread_args*)p)->thread_end_timestamp > ((struct thread_args*)q)->thread_end_timestamp) {return 1;}
   else {return 0;}
} 

int main(int argc, char **argv)
{
	responseTimeArray = (long long*) malloc(50 * sizeof(long long));
	deadlineArray = (long long*) malloc(50 * sizeof(long long));
	expectReleaseTime = (long long*) malloc(50 * sizeof(long long));
	actualReleaseTime = (long long*) malloc(50 * sizeof(long long));
	diffReleaseTime = (long long*) malloc(50 * sizeof(long long));
	/*<======== You have to change the following three data structures whose values will be assigned to threads in sequence =========>*/    
	/* NOTE: Do not forget to change the value of macro NUM_THREADS when you want to change the number of threads, 
	 * and then fill the following arrays accordingly. Default value of NUM_THREADS is 4. 
	 * -------------------------------------------------------------------------------------------------------------------------------
	 * Warning: If you run the code without filling in priorities, you will get runtime stack error.
	 * If you do not have a multicore computer, then change the value of affinities[i] from 1 to 0.
	 */

	int priorities[NUM_THREADS]; /* Priorities are in the range from 1 to 99 when it comes to SCHED_FIFO and SCHED_RR, and for  
	SCHED_OTHER, the priority is always 0 */

	int periods[NUM_THREADS];	// Used in calculation of next period value in a periodic task / thread.

	// below for RR (Not anymore)

	// priorities[0] = 3;
	// periods[0] = 675000; //150000*4.5 

	// priorities[1] = 3;
	// periods[1] = 825000; //150000*5.5

	// priorities[2] = 2;
	// periods[2] = 975000; //150000*6.5 

	// priorities[3] = 2;
	// periods[3] = 1125000; //150000*7.5 

	// priorities[4] = 1;
	// periods[4] = 1275000; //150000*8.5 

	// priorities[5] = 1;
	// periods[5] = 1425000; //150000*9.5 

	// //total utilization = 0.9141301079



	//below for FIFO and RR

	priorities[0] = 6;
	periods[0] = 22500; //150000*4.5 

	priorities[1] = 5;
	periods[1] = 27500; //150000*5.5

	priorities[2] = 4;
	periods[2] = 32500; //150000*6.5 

	priorities[3] = 3;
	periods[3] = 37500; //150000*7.5 

	priorities[4] = 2;
	periods[4] = 42500; //150000*8.5 

	priorities[5] = 1;
	periods[5] = 47500; //150000*9.5 

	// total utilization = 0.9141301079


	

	//below for FIFO and RR (task not in the correct prioirty)

	// priorities[1] = 3;
	// periods[1] = 22500; //150000*4.5 

	// priorities[0] = 3;
	// periods[0] = 27500; //150000*5.5

	// priorities[3] = 2;
	// periods[3] = 32500; //150000*6.5 

	// priorities[2] = 2;
	// periods[2] = 37500; //150000*7.5 

	// priorities[5] = 1;
	// periods[5] = 42500; //150000*8.5 

	// priorities[4] = 1;
	// periods[4] = 47500; //150000*9.5  

	// //total utilization = 0.867159


	//below for OTHER

	// priorities[0] = 0;
	// periods[0] = 22500; //150000*4.5 

	// priorities[1] = 0;
	// periods[1] = 27500; //150000*5.5

	// priorities[2] = 0;
	// periods[2] = 32500; //150000*6.5 

	// priorities[3] = 0;
	// periods[3] = 37500; //150000*7.5 

	// priorities[4] = 0;
	// periods[4] = 42500; //150000*8.5 

	// priorities[5] = 0;
	// periods[5] = 47500; //150000*9.5 

	// // total utilization = 0.9141301079

	struct timespec wakeup_time;
  	clock_gettime(CLOCK_REALTIME, &wakeup_time);
	timespec_add_us(&wakeup_time, 10000);

	/*<======== Do not change anything below unless you have to change value of affinities[i] below =========>*/
	
	thread_func_prototype thread_functions[NUM_THREADS] = {Thread};
	pthread_t thread_id[NUM_THREADS];
	pthread_attr_t attributes[NUM_THREADS];
	int affinities[NUM_THREADS];
	int policies[NUM_THREADS];
	struct thread_args lvargs[NUM_THREADS] = {0};
	srand((unsigned) time(NULL));

	if(argc <= 1)
	{
		help();
		return FALSE; 
	} else {
		if(strcmp(argv[1], "RR") == 0) {
			printf("Round Robin Scheduling\n");
			for(int i = 0; i < NUM_THREADS; i++) {
				policies[i] = SCHED_RR;
				inputpolicy = SCHED_RR;
				affinities[i]	=	1;
			}
		} else if(strcmp(argv[1], "FIFO") == 0) {
			printf("First in First out Scheduling\n");
			for(int i = 0; i < NUM_THREADS; i++) {
				policies[i] = SCHED_FIFO;
				inputpolicy = SCHED_FIFO;
				affinities[i]	=	1; 
			}
		} else if(strcmp(argv[1], "OTHER") == 0) {
			printf("Completely Fair Scheduling\n");
			for(int i = 0; i < NUM_THREADS; i++) {				
				policies[i] = SCHED_OTHER;
				inputpolicy = SCHED_OTHER;
				affinities[i]	=	1;
			}
		} else {
			help();
			return FALSE;
		}
	}

	printf("Assignment_2 Threads:%d Scheduling Policy:%s\n", NUM_THREADS, POL_TO_STR(inputpolicy));

	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset);
	// For Main Thread Affinity is set to CPU 0
	if(sched_setaffinity(0, sizeof(cpuset), &cpuset) != 0) {printf("%d Main Error Setting CPU Affinity\n", __LINE__);}

	trace_fd = open("/sys/kernel/debug/tracing/trace_marker", O_WRONLY);
	if(trace_fd < 0) {
		printf("Error Opening trace marker, try running with sudo\n");
	} else{
		printf("Successfully opened trace_marker\n");
	}

	if((set_attributes(priorities, NUM_THREADS, affinities, policies, attributes) == FALSE))
	{
		printf("Error Setting Attributes of all threads...\n");
	}

	for(int i = 0; i < NUM_THREADS; i++)
	{
		lvargs[i].thread_number = i;
		lvargs[i].thread_policy = policies[i];
		lvargs[i].thread_affinity = affinities[i];
		lvargs[i].thread_period = periods[i];
		lvargs[i].thread_priority = priorities[i];
		lvargs[i].wake_time = wakeup_time;

		if(pthread_create(&thread_id[i], &attributes[i], Thread, (void*)&lvargs[i]) != 0)
		{
			printf("%d - %s %d\n", __LINE__, "Error Spawning Thread:", i);
		}
		else
		{
			trace_write("RTS_Spawned Thread_%d\n", i);
		}	
	}
	
	for(int i=0; i<NUM_THREADS; i++) {
		pthread_join(thread_id[i], NULL);
	}

	// below is the code for reversing the creation of threads

	// for(int i=NUM_THREADS-1; i>=0; i--)
	// {
	// 	lvargs[i].thread_number = i;
	// 	lvargs[i].thread_policy = policies[i];
	// 	lvargs[i].thread_affinity = affinities[i];
	// 	lvargs[i].thread_period = periods[i];
	// 	lvargs[i].thread_priority = priorities[i];
	// 	lvargs[i].wake_time = wakeup_time;

	// 	if(pthread_create(&thread_id[i], &attributes[i], Thread, (void*)&lvargs[i]) != 0)
	// 	{
	// 		printf("%d - %s %d\n", __LINE__, "Error Spawning Thread:", i);
	// 	}
	// 	else
	// 	{
	// 		trace_write("RTS_Spawned Thread_%d\n", i);
	// 	}	
	// }

	// for(int i=NUM_THREADS-1; i>=0; i--) {
	// 	pthread_join(thread_id[i], NULL);
	// }

	qsort(results, NUM_THREADS, sizeof(struct thread_args), comparator);
	
	for(int i=0;i<NUM_THREADS;i++)
	{
		results[i].thread_exectime = (clock_diff(results[i].thread_start_time, results[i].thread_end_time)/1.0e6);
		printf(
				"--- >>> TimeStamp:%llu Thread:%d Policy:%s Affinity:%d Priority:%d  ExecutionTime:%f Responsetime:%lldms Deadline:%lld\n",
				results[i].thread_end_timestamp, results[i].thread_number, POL_TO_STR(results[i].thread_policy), 
				results[i].thread_affinity, results[i].thread_priority, results[i].thread_exectime, 
				results[i].thread_response_time, results[i].thread_deadline
			  );
	}

	if(strcmp(argv[1], "RR") == 0) {

		FILE* out = fopen("RR-responseTimeArray.txt", "w");

		// Write the contents of the array to the file
		for (int i = 0; i < 50; i++) {
			fprintf(out, "%lld\n", responseTimeArray[i]);
		}

		// Close the file
		fclose(out);

		FILE* out1 = fopen("RR-deadlineArray.txt", "w");

		// Write the contents of the array to the file
		for (int i = 0; i < 50; i++) {
			fprintf(out1, "%lld\n", deadlineArray[i]);
		}

		// Close the file
		fclose(out1);
			
	} else if(strcmp(argv[1], "FIFO") == 0) {

		FILE* out = fopen("FIFO-responseTimeArray.txt", "w");

		// Write the contents of the array to the file
		for (int i = 0; i < 50; i++) {
			fprintf(out, "%lld\n", responseTimeArray[i]);
		}

		// Close the file
		fclose(out);

		FILE* out1 = fopen("FIFO-deadlineArray.txt", "w");

		// Write the contents of the array to the file
		for (int i = 0; i < 50; i++) {
			fprintf(out1, "%lld\n", deadlineArray[i]);
		}

		// Close the file
		fclose(out1);
		
	} else if(strcmp(argv[1], "OTHER") == 0) {

		FILE* out = fopen("OTHER-responseTimeArray.txt", "w");

		// Write the contents of the array to the file
		for (int i = 0; i < 50; i++) {
			fprintf(out, "%lld\n", responseTimeArray[i]);
		}

		// Close the file
		fclose(out);

		FILE* out1 = fopen("OTHER-deadlineArray.txt", "w");

		// Write the contents of the array to the file
		for (int i = 0; i < 50; i++) {
			fprintf(out1, "%lld\n", deadlineArray[i]);
		}

		// Close the file
		fclose(out1);

	} else {}

	for (int i = 0; i < 50; i++) {
		diffReleaseTime[i] = actualReleaseTime[i] - expectReleaseTime[i];
	}
	FILE* out = fopen("ReleaseTimeDiff.txt", "w");
	// Write the contents of the array to the file
	for (int i = 0; i < 50; i++) {
		fprintf(out, "%lld\n", diffReleaseTime[i]);
	}
	// Close the file
	fclose(out);


	if(close(trace_fd) == 0)
	{
		printf("Successfully closed the trace file\n");
	}

	free(responseTimeArray);
	free(deadlineArray);
	free(expectReleaseTime);
	free(actualReleaseTime);
	free(diffReleaseTime);
	return 0;
}