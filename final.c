/*
	Test suite for perfromance measurements as part of CSC 501
	Author: Abhinav Choudhury, Aditya Virmani
*/

#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

typedef unsigned long long uint64_t;

#define DECLARE_COUNTERS	unsigned cycles_high, cycles_low, cycles_high1, cycles_low1;
#define START()	{ \
	asm volatile ( \
			"CPUID\n\t" \
			"RDTSC\n\t" \
			"mov %%edx, %0\n\t" \
			"mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low) :: "%rax", "%rbx", "%rcx", "%rdx"); \
	}

#define STOP()  { \
	asm volatile ( \
			"RDTSCP\n\t" \
			"mov %%edx, %0\n\t" \
			"mov %%eax, %1\n\t" \
			"CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1) :: "%rax", "%rbx", "%rcx", "%rdx"); \
	}

#define ELAPSED_CYCLES()	( ( ((uint64_t)cycles_high1 << 32) | cycles_low1 ) - ( ((uint64_t)cycles_high << 32) | cycles_low ) )

/* measurement_overhead is a global that is initialized when 
	measurement overhead calculations are done in 
	calculate_measurement_overhead() function. This needs to be 
	subtracted from all subsequent variables that we measure.
*/
bool measurement_overhead_initialized = false;
uint64_t measurement_overhead;
bool cpu_cycles_per_sec_inited = false;
uint64_t cpu_cycles_per_sec;



double calculate_sd(int count, uint64_t* values, double mean)
{
	double sum = 0;
	int i = 0;

	for (i = 0; i < count; i++)
	{
		sum += (values[i] - mean)* (values[i] - mean);
	}

	sum = (double)(sum/(double)(count - 1));

	return (sqrt(sum));
}


/* Test functions defined below */

#define CPU_FREQ_ITERATIONS	2
long measure_CPU_frequency()
{
	DECLARE_COUNTERS
	int i = 0;
	uint64_t sum = 0;
	uint64_t times[CPU_FREQ_ITERATIONS];

	printf("\nPHASE 1: Measuring CPU frequency...\n");

	for (i = 0; i < CPU_FREQ_ITERATIONS; i++)
	{
		START();
		sleep(1);	// Sleep for 1 sec
		STOP();
	
		sum += ELAPSED_CYCLES();
		times[i] = ELAPSED_CYCLES();

		printf(".\n");
	}

	cpu_cycles_per_sec = sum/CPU_FREQ_ITERATIONS;
	cpu_cycles_per_sec_inited = true;
	printf("CPU frequency: %llu cycles per sec or %lf GHz\n", cpu_cycles_per_sec, (double)cpu_cycles_per_sec/(double)10000000000L);
	printf("CPU cycle time: %lf nsecs\n", (double)(1000000000)/(double)(cpu_cycles_per_sec) );
	printf("SD is: %lf\n", calculate_sd(CPU_FREQ_ITERATIONS, times, cpu_cycles_per_sec));

	return (long)cpu_cycles_per_sec;
}

#define OVERHEAD_MEASUREMENT_ITERS	1000
void calculate_measurement_overhead()
{
	DECLARE_COUNTERS
	int i = 0;
	uint64_t sum = 0;
	uint64_t times[OVERHEAD_MEASUREMENT_ITERS];

	printf("\nPHASE 2: Estimating measurement overhead...\n");

	for (i =0; i < OVERHEAD_MEASUREMENT_ITERS; i++)
	{
		START();
		STOP();

		sum += ELAPSED_CYCLES();
		times[i] = ELAPSED_CYCLES();
	}

	measurement_overhead = sum/OVERHEAD_MEASUREMENT_ITERS;
	measurement_overhead_initialized = true;
	printf("Measurement overhead is %llu cycles\n", sum/OVERHEAD_MEASUREMENT_ITERS);
	printf("Measurement overhead time is: %lf nsecs\n",
		 (double)(sum*1000000000/OVERHEAD_MEASUREMENT_ITERS)/(double)(cpu_cycles_per_sec) );
	printf("Standard deviation for measurement overhead: %lf cycles or %lf nsec\n",
		 calculate_sd(OVERHEAD_MEASUREMENT_ITERS, times, measurement_overhead),
		 (double)calculate_sd(OVERHEAD_MEASUREMENT_ITERS, times, measurement_overhead)*(double)1000000000/cpu_cycles_per_sec);
}


int __attribute__((optimize("O0"))) func()
{
	return 0;
}

int __attribute__((optimize("O0"))) func1(int a)
{
	return (a+a);
}

int __attribute__((optimize("O0"))) func2(int a, int b)
{
    return (a+b);
}

int __attribute__((optimize("O0"))) func3(int a, int b, int c)
{
    return (a+b+c);
}

int __attribute__((optimize("O0"))) func4(int a, int b, int c, int d)
{
    return (a+b+c+d);
}

int __attribute__((optimize("O0"))) func5(int a, int b, int c, int d, int e)
{
    return (a+b+c+d+e);
}

int __attribute__((optimize("O0"))) func6(int a, int b, int c, int d, int e, int f)
{
    return (a+b+c+d+e+f);
}

int __attribute__((optimize("O0"))) func7(int a, int b, int c, int d, int e, int f, int g)
{
    return (a+b+c+d+e+f+g);
}


/* Mean calculator */
double calculate_mean(int count, uint64_t* values)
{
	double sum = 0;
	int i = 0;

	for (i = 0; i < count; i++)
	{
		sum += values[i];
	}

	return (sum/(double)count);
}

/* For procedure calls, we would expect the first call to take much higher number of cycles
	as compared to subsequent calls, since the first time, the procedure instructions are not
	in cache. Once called, the function instructions are loaded into the cache, it takes
	lesser cycles to call the function.
*/
#define PROCCALL_MEASUREMENT_ITERS	100000
void calculate_proccall_overhead()
{
	DECLARE_COUNTERS;
	int i = 0;
	uint64_t sum = 0;
	uint64_t times[8][PROCCALL_MEASUREMENT_ITERS];

	printf("\nPHASE 3: Running procedure call tests...\n");

	if (!measurement_overhead_initialized)
	{
		printf("%s WARNING: Measurement overhead variable not initialized. Bailing out...\n", __FUNCTION__);
		return;
	}

	FILE *fp = fopen("proccall_measurement_data.txt", "w");

	if (fp == NULL)
	{
		printf("%s WARNING: Failed to open file\n", __FUNCTION__);
	}

	fprintf(fp, "ARG0 ARG1 ARG2 ARG3 ARG4 ARG5 ARG6 ARG7\n");

	// Warm-up. If we don't warmup, the first calls will take
	// a lot of extra cycles spent in loading/caching the function instructions
	func();
	func1(1);
	func2(1, 2);
	func3(1, 2, 3);
	func4(1, 2, 3, 4);
	func5(1, 2, 3, 4, 5);
	func6(1, 2, 3, 4, 5, 6);
	func7(1, 2, 3, 4, 5, 6, 7);

	for (i = 0; i < PROCCALL_MEASUREMENT_ITERS; i++)
	{
		START();
		func();
		STOP();
		fprintf(fp, "%llu ", ELAPSED_CYCLES() - measurement_overhead);
		times[0][i] = ELAPSED_CYCLES();

		START();
		func1(i);
		STOP();
		fprintf(fp, "%llu ", ELAPSED_CYCLES() - measurement_overhead);
		times[1][i] = ELAPSED_CYCLES();

		START();
		func2(i, i+1);
		STOP();
		fprintf(fp, "%llu ", ELAPSED_CYCLES() - measurement_overhead);
		times[2][i] = ELAPSED_CYCLES();

		START();
		func3(i, i+1, i+2);
		STOP();
		fprintf(fp, "%llu ", ELAPSED_CYCLES() - measurement_overhead);
		times[3][i] = ELAPSED_CYCLES();

		START();
		func4(i, i+1, i+2, i+3);
		STOP();
		fprintf(fp, "%llu ", ELAPSED_CYCLES() - measurement_overhead);
		times[4][i] = ELAPSED_CYCLES();

		START();
		func5(i, i+1, i+2, i+3, i+4);
		STOP();
		fprintf(fp, "%llu ", ELAPSED_CYCLES() - measurement_overhead);
		times[5][i] = ELAPSED_CYCLES();

		START();
		func6(i, i+1, i+2, i+3, i+4, i+5);
		STOP();
		fprintf(fp, "%llu ", ELAPSED_CYCLES() - measurement_overhead);
		times[6][i] = ELAPSED_CYCLES();

		START();
		func7(i, i+1, i+2, i+3, i+4, i+5, i+6);
		STOP();
		fprintf(fp, "%llu\n", ELAPSED_CYCLES() - measurement_overhead);
		times[7][i] = ELAPSED_CYCLES();
	}

	for (i = 0; i < 8; i++)
	{
		double mean = calculate_mean(PROCCALL_MEASUREMENT_ITERS, times[i]);
		double sd = calculate_sd(PROCCALL_MEASUREMENT_ITERS, times[i], mean);

		printf("Average cycles for procedure call with %d arguments: %lf, %lf nsecs, SD : %lf\n",
			i, mean, mean*1000000000/cpu_cycles_per_sec, sd);
	}
}

/* Since system calls are sometimes cached and do not trap into the kernel
   we spawn multiple processes to make the system call separately
   and measure the time
*/
#define SYSCALL_MEASUREMENT_ITERS	1000
void calculate_syscall_overhead()
{
	DECLARE_COUNTERS;
	int i = 0, status;
	uint64_t sum = 0, elapsed;
	int pipe1[2];
	uint64_t times[SYSCALL_MEASUREMENT_ITERS];

	printf("\nPHASE 4: Running syscall tests...\n");

	FILE *fp = fopen("syscall_measurements_output.txt", "w");

	if (pipe(pipe1) == -1)
	{
		printf("Error creating pipe...\n");
	}

	if (fp == NULL)
	{
		printf("%s WARNING: Failed to open file\n", __FUNCTION__);
	}

	for (i = 0; i < SYSCALL_MEASUREMENT_ITERS; i++)
	{
		pid_t pid = fork();
		if (pid == 0)	// Child
		{
			START();
			getpid();
			STOP();

			elapsed = ELAPSED_CYCLES();
			write(pipe1[1], &elapsed, sizeof(elapsed));
			exit(1);
		}
		else 	// Parent
		{
			read(pipe1[0], &elapsed, sizeof(elapsed));
			times[i] = elapsed;
			wait(&status);
		}
	}

	double mean = calculate_mean(SYSCALL_MEASUREMENT_ITERS, times);
	double sd = calculate_sd(SYSCALL_MEASUREMENT_ITERS, times, mean);

	printf("Overhead of making a system call is %lf cycles or %lf nsecs, SD: %lf\n", mean, mean*1000000000/cpu_cycles_per_sec, sd);
}

#define PC_OVERHEAD_ITERS 1000
void calculate_process_creation_overhead()
{
	DECLARE_COUNTERS;
	int i = 0, status;
	uint64_t sum = 0;
	uint64_t times[PC_OVERHEAD_ITERS];

	printf("\nPHASE 5: Running process creation tests...\n");

	for ( i=0; i<PC_OVERHEAD_ITERS; i++)
	{
		pid_t pid;

		START();
		pid = fork();

		if (pid == 0)	// If child, do nothing
		{
			exit(1);
		}
		else 	// If parent, stop counter first, then wait for child
		{
			STOP();
			wait(NULL);
			times[i] = ELAPSED_CYCLES();
		}
	}

	double mean = calculate_mean(PC_OVERHEAD_ITERS, times);
	double sd = calculate_sd(PC_OVERHEAD_ITERS, times, mean);

	printf("Average process creation overhead: %lf cycles, %lf nsecs, SD: %lf\n",
		mean, mean*1000000000/cpu_cycles_per_sec, sd);
}

void * thread_func(void *arg)
{
	return 0;
}

#define TC_OVERHEAD_ITERS 1000
void calculate_thread_creation_overhead()
{
	DECLARE_COUNTERS;
	int i = 0;
	uint64_t sum = 0;
	pthread_t thread;
	uint64_t times[TC_OVERHEAD_ITERS];

	printf("\nPHASE 6: Running thread creation tests...\n");

	for (i = 0; i < TC_OVERHEAD_ITERS; i++)
	{
		START();
		if (0 == pthread_create(&thread, NULL, thread_func, NULL))
		{
			STOP();
			if (pthread_join(thread, NULL))
			{
				printf("Error joining thread...\n");
			}
			times[i] = ELAPSED_CYCLES();
		}
	}

	double mean = calculate_mean(TC_OVERHEAD_ITERS, times);
	double sd = calculate_sd(TC_OVERHEAD_ITERS, times, mean);

	printf("Average thread creation overhead: %lf cycles, %lf nsecs, SD: %lf\n",
		mean, mean*1000000000/cpu_cycles_per_sec, sd);
}

#define CONTEXT_SWITCH_MEASUREMENT_ITERS 1000
void calculate_context_switch_time()
{
	DECLARE_COUNTERS;
//	cpu_set_t affinity;
//	struct sched_param prio_params;
//	int max_prio;
	uint64_t starttime, timevar, endtime;
	int pipe1[2], pipe2[2];
	pid_t pid;
	uint64_t times[CONTEXT_SWITCH_MEASUREMENT_ITERS];
	int i = 0;

	printf("\nPHASE 7: Running context switch tests...\n");

	if (pipe(pipe1) == -1)
	{
		printf("Pipe creation failed...\n");
	}

	if (pipe(pipe2) == -1)
	{
		printf("Pipe creation failed...\n");
	}

	for(i = 0; i < CONTEXT_SWITCH_MEASUREMENT_ITERS; i++)
	{
		pid = fork();
		if (pid == 0)	// Child
		{
			STOP();

			if (read(pipe1[0], (char*)&timevar, sizeof(timevar)) == -1)
			{
				printf("CHILD: read error!\n");
			}

			uint64_t elapsed = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 ) - timevar;

			if (write(pipe2[1], (char*)&elapsed, sizeof(elapsed)) == -1)
			{
				printf("CHILD: write error!\n");
			}

			exit(1);
		}
		else 	// Parent
		{
			START();
			starttime = ( ((uint64_t)cycles_high << 32) | cycles_low );
			if (write(pipe1[1], &starttime, sizeof(starttime)) == -1)
			{
				printf("PARENT: Write error!\n");
			}
			if (read(pipe2[0], &timevar, sizeof(timevar)) == -1)
			{
				printf("PARENT: Read error!\n");
			}

	//		printf("TIMEVAR: %llu\n", timevar);
			times[i] = timevar;
			wait(NULL);
		}
	}

	double mean = calculate_mean(CONTEXT_SWITCH_MEASUREMENT_ITERS, times);
	double sd = calculate_sd(CONTEXT_SWITCH_MEASUREMENT_ITERS, times, mean);

	printf("Average context switch time: %lf cycles, %lf nsecs, SD: %lf\n",
		mean, mean*1000000000/cpu_cycles_per_sec, sd);
}

// Calculating memory bandwidth
#define MEMRW_BANDWIDTH_ITERS	100
void calculate_memory_rw_bandwidth()
{
	DECLARE_COUNTERS;
	int **alloced_mem = NULL;
	int i = 0, j = 0;
	int sum = 0;
	uint64_t cycle_sum = 0;

	printf("\nPHASE 9: Running memory r/w bandwidth tests...\n");

	alloced_mem = calloc(7, sizeof(int*));
	if (alloced_mem == NULL)
	{
		printf("Mem alloc failure\n");
	}

	// Memory read bandwidth
	for (i = 0; i < 7; i++)
	{
		int mem_to_alloc = 512*1024*pow(2, i);
		alloced_mem[i] = malloc(mem_to_alloc);

		if (alloced_mem[i] == NULL)
			printf("Mem alloc failure for %d bytes\n", mem_to_alloc);

		int *startptr = alloced_mem[i];
		int *endptr = startptr + mem_to_alloc/sizeof(int);
		for (j = 0; j < MEMRW_BANDWIDTH_ITERS; j++)
		{
			int* p = startptr;
			START();
			while (p < endptr)
			{
				sum += p[0] + p[4] + p[8] + p[12] + p[16] + p[20] + p[24] + p[28] + p[32]
					+ p[36] + p[40] + p[44] + p[48] + p[52] + p[56] + p[60] + p[64] + p[68]
					+ p[72] + p[76] + p[80] + p[84] + p[88] + p[92] + p[96] + p[100] + p[104]
					+ p[108] + p[112] + p[116] + p[120] + p[124];
				p = p + 128;
			}
			STOP();
			cycle_sum += ELAPSED_CYCLES();
		}
			uint64_t average_cycles = cycle_sum/MEMRW_BANDWIDTH_ITERS;
			printf("Average cycles elapsed for reading %d bytes (%lf MB) is %llu\n", mem_to_alloc, (double)mem_to_alloc/((double)1024*1024), ELAPSED_CYCLES());
			printf("The corresponding time required is: %lf secs\n", (double)average_cycles/(double)cpu_cycles_per_sec);
		printf("\n");
	}

	// Memory write bandwidth
	for (i = 0; i < 7; i++)
	{

		int *startptr = alloced_mem[i];
		int *endptr = startptr + mem_to_alloc/sizeof(int);
		for (j = 0; j < MEMRW_BANDWIDTH_ITERS; j++)
		{
			int* p = startptr;
			START();
			while (p < endptr)
			{
				p[0] = 1; p[4] = 1; p[8] = 1; p[12] = 1; p[16] = 1; p[20] = 1;
				p[24] = 1; p[28] = 1; p[32] = 1; p[36] = 1; p[40] = 1; p[44] = 1;
				p[48] = 1; p[52] = 1; p[56] = 1; p[60] = 1; p[64] = 1; p[68] = 1;
				p[72] = 1; p[76] = 1; p[80] = 1; p[84] = 1; p[88] = 1; p[92] = 1;
				p[96] = 1; p[100] = 1; p[104] = 1; p[108] = 1; p[112] = 1; p[116] = 1;
				p[120] = 1; p[124] = 1;
				p = p + 128;
			}
			STOP();
			cycle_sum += ELAPSED_CYCLES();
		}
			uint64_t average_cycles = cycle_sum/MEMRW_BANDWIDTH_ITERS;
			printf("Average cycles elapsed for writing %d bytes (%lf MB) is %llu\n", mem_to_alloc, (double)mem_to_alloc/((double)1024*1024), ELAPSED_CYCLES());
			printf("The corresponding time required is: %lf secs\n", (double)average_cycles/(double)cpu_cycles_per_sec);
		printf("\n");
	}
}

// Calculate Memory Latency
#define ONE iterate = (long *) *iterate;
#define FIVE ONE ONE ONE
#define TWOFIVE FIVE FIVE FIVE FIVE FIVE
#define HUNDRED TWOFIVE TWOFIVE TWOFIVE TWOFIVE
#define TWOFIFTY HUNDRED HUNDRED TWOFIVE TWOFIVE
#define THOUSAND TWOFIFTY TWOFIFTY TWOFIFTY TWOFIFTY

int step(int base){
    if(base < 16*1024) {
        return 1024;
    }
    int step_size = 1024;
    while (step_size <= base) {
        step_size *= 2;
    }
    return step_size/16;
}

void calculate_latency_size(long *head, int limit){
    DECLARE_COUNTERS
    int num_entries, r, idx, back_num, backup, i;
    num_entries = limit/sizeof(int*);
    int *i_array = (int *) malloc(num_entries * sizeof(int));
    for (i = 0;i < num_entries;i++) {
        *(i_array + i) = i;
    }
    srand(0);
    back_num = num_entries - 1;
    for (i = 0;i < num_entries;i++) {
        r = rand();
        idx = r%num_entries;
        backup = *(i_array + idx);
        *(i_array + idx) = *(i_array + back_num);
        *(i_array + back_num) = backup;
        --back_num;
    }
    for (i = 0;i < num_entries;i++) {
        head[i] = (long *) &head[i_array[i]];
    }
    free(i_array);
    i = 1000000;
    long *iterate;
    iterate = head;
    long dummy = 0;
    START()
    while (i > 0) {
        THOUSAND
        i -= 1000;
    }
    STOP()
    dummy += *iterate;
    long num_cycles = ELAPSED_CYCLES();
    printf("size = %d, cycles = %lf, dummy value (ignore) = %ld \n",limit/1024,num_cycles/1000000.,dummy);
}

void calculate_memory_latency (long cycles)
{
        int base = 1024;
        int limit = 64*1024*1024;
        long *array = (long *) malloc(limit);

        printf("\nPHASE 8: Running memory latency tests...\n");

        for (int i = base;i < limit;i = i + step(i)){
            calculate_latency_size(array, i);
        }
}
// End Memory Latency

// Begin Page Fault Service time
void calculate_page_fault_service_time(long cpu_freq) {
    DECLARE_COUNTERS
    printf("\nPHASE 10: Running page fault tests...\n");
    int fd = open("random", O_RDWR);
    long read_size = 6;
    read_size = read_size*1024*1024*1024;
    long page_size = sysconf(_SC_PAGESIZE);
    char * random_data = mmap(
                              NULL
                              , read_size
                              , PROT_READ | PROT_WRITE
                              , MAP_SHARED
                              , fd
                              , 0
                              ); // get some random data
    //char* random_data =(char*) mmap(NULL, read_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    if (random_data == MAP_FAILED) {
        printf("read error");
        return;
    }
    long num_access = read_size/page_size;
    int * cycle_counts = (int *) malloc(sizeof(int) * num_access);
    size_t i,j=0;;
    
    long back_num, backup, r, idx;
    int *i_array = (int *) malloc(num_access * sizeof(int));
    for (i = 0;i < num_access;i++) {
        *(i_array + i) = i;
    }
    srand(0);
    back_num = num_access - 1;
    for (i = 0;i < num_access;i++) {
        r = rand();
        idx = r%num_access;
        backup = *(i_array + idx);
        *(i_array + idx) = *(i_array + back_num);
        *(i_array + back_num) = backup;
        --back_num;
    }
    for (i = 0; i < read_size; i += page_size) {
        idx = i_array[i/page_size];
        idx *= page_size;
        START()
        random_data[idx] = 1;
        STOP()
	printf("\rIn progress %.1lf %%", ((double)j*100)/num_access);
        //printf("j = %ld, num_access = %ld \n",j,num_access);
        cycle_counts[j] = ELAPSED_CYCLES();
        j++;
	fflush(stdout);
    }
    free(i_array);
    long sum = 0;
    for (i = 0; i<num_access;i++)
        sum += cycle_counts[i];
    double cycles = (double)sum/num_access;
    double time = cycles * 1000/(cpu_freq);
    printf("average cycle time %lf, and time is %lf ms\n",cycles, time);
}
// End Page Fault Service time

int main(int argc, int* argv)
{
	cpu_set_t affinity;
	struct sched_param prio_params;
	int max_prio;

	CPU_ZERO(&affinity);
	CPU_SET(0, &affinity);

	if (sched_setaffinity(getpid(), sizeof(cpu_set_t), &affinity))
	{
		printf("Setting CPU affinity for process %d failed\n", getpid());
	}

	if ((max_prio = sched_get_priority_max(SCHED_FIFO)) < 0)
	{
		printf("Get max priority for process failed\n");
	}

//	prio_params.sched_priority = max_prio;
	int ret = sched_setscheduler(getpid(), SCHED_FIFO, &prio_params);
//	if (sched_setscheduler(getpid(), SCHED_FIFO, &prio_params))
	if (ret == EPERM)
	{
		printf("Setting policy/priority for process failed: EPERM\n");
	}
	else if (ret == EINVAL)
		printf("EINVAL\n");
	else
		printf("PID: %d ESRCH\n", getpid());


	/* PHASE 1: Calculate the base frequency of the processor */
	measure_CPU_frequency();

	/* PHASE 2: Estimate the measurement overhead */
	calculate_measurement_overhead();

	/* PHASE 3: Estimate the overhead of making a procedure call */
	calculate_proccall_overhead();

	/* PHASE 4: Estimate the overhead of making a system call */
	calculate_syscall_overhead();

	/* PHASE 5: Compute time to create a process */
	calculate_process_creation_overhead();

	/* PHASE 6: Calculate thread creation overhead */
	calculate_thread_creation_overhead();

	/* PHASE 7: Calculate context swicth time */
	calculate_context_switch_time();

    /* PHASE 8: Calculate memory latency time */
    calculate_memory_latency(measure_CPU_frequency());

	/* PHASE 9: Calculate memory read/write bandwidth */
	calculate_memory_rw_bandwidth();

    /* PHASE 10: Calculate Page Fault Service Time */
    calculate_page_fault_service_time(measure_CPU_frequency());
    
    return 0;
}
