#include <unistd.h>
#include <stdlib.h>

typedef unsigned long long uint64_t;

#define DECLARE_COUNTERS	uint64_t start, end; unsigned cycles_high, cycles_low, cycles_high1, cycles_low1;
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

#define CPU_FREQ_ITERATIONS	10
void measure_CPU_frequency()
{
	DECLARE_COUNTERS
	int i = 0;
	uint64_t sum = 0;

	printf("Measuring CPU frequency...\n");

	for (i = 0; i < CPU_FREQ_ITERATIONS; i++)
	{
		START();
		sleep(1);	// Sleep for 1 sec
		STOP();
	
		sum += ELAPSED_CYCLES();
		printf("%i: Elapsed cycles = %llu\n", i+1, ELAPSED_CYCLES());
	}
	
	printf("CPU frequency: %llu per sec or %llu GHz\n", sum, sum/10000000000L);
}

#define OVERHEAD_MEASUREMENT_ITERS	10
void calculate_measurement_overhead()
{
	DECLARE_COUNTERS
	int i = 0;
	uint64_t sum = 0;

	printf("Computing measurement overhead...\n");

	for (i =0; i < OVERHEAD_MEASUREMENT_ITERS; i++)
	{
		START();
		STOP();

		sum += ELAPSED_CYCLES();
		printf("%i: Elapsed cycles = %llu\n", i+1, ELAPSED_CYCLES());
	}

	printf("Measurement overhead is %llu cycles\n", sum/OVERHEAD_MEASUREMENT_ITERS);
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

/* For procedure calls, we would expect the first call to take much higher number of cycles
	as compared to subsequent calls, since the first time, the procedure instructions are not
	in cache. Once called, the function instructions are loaded into the cache, it takes
	lesser cycles to call the function.
*/
#define PROCCALL_MEASUREMENT_ITERS	10
void calculate_proccall_overhead()
{
	DECLARE_COUNTERS;
	int i = 0;
	uint64_t sum = 0;

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

	for (i = 0; i < PROCCALL_MEASUREMENT_ITERS; i++)
	{
		START();
		func();
		STOP();
		fprintf();

		START();
		func1(i);
		STOP();

		START();
		func2(i, i+1);
		STOP();

		START();
		func3(i, i+1, i+2);
		STOP();

		START();
		func4(i, i+1, i+2, i+3);
		STOP();

		START();
		func5(i, i+1, i+2, i+3, i+4);
		STOP();

		START();
		func6(i, i+1, i+2, i+3, i+4, i+5);
		STOP();

		START();
		func7(i, i+1, i+2, i+3, i+4, i+5, i+6);
		STOP();
	}

}

/* Since system calls are sometimes cached and do not trap into the kernel
   we spawn multiple processes to make the system call separately
   and measure the time
*/
#define SYSCALL_MEASUREMENT_ITERS	10
void calculate_syscall_overhead()
{
	DECLARE_COUNTERS;
	int i = 0;
	uint64_t sum = 0;

	printf("Computing syscall overhead...\n");

	FILE *fp = fopen("syscall_measurements_output.txt", "w");

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

			fprintf(fp, "%llu\n", ELAPSED_CYCLES());
			printf("Elapsed cycles: %llu\n", ELAPSED_CYCLES());
		}
		else 	// Parent
		{
			wait();
		}
	}

	printf("Overhead of making a system call is %llu cycles\n", sum/SYSCALL_MEASUREMENT_ITERS);
}

#define PC_OVERHEAD_ITERS 10
void calculate_process_creation_overhead()
{
	DECLARE_COUNTERS;
	int i = 0;
	uint64_t sum = 0;

	FILE *fp = fopen("process_creation_data.txt", "w");

	if (fp == NULL)
	{
		printf("%s WARNING: Failed to open file...\n", __FUNCTION__);
	}

	for ( i=0; i<PC_OVERHEAD_ITERS; i++)
	{
		pid_t pid;

		START();
		pid = fork();

		if (pid == 0)	// If child, do nothing
		{
			return;
		}
		else 	// If parent, stop counter first, then wait for child
		{
			STOP();
			wait();
			fprintf(fp, "%llu\n", ELAPSED_CYCLES());
		}
	}
}

#define TC_OVERHEAD_ITERS 10
void calculate_thread_creation_overhead()
{

}

#define CONTEXT_SWITCH_MEASUREMENT_ITERS 10
void calculate_context_switch_time()
{

}

int main(int argc, int* argv)
{
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

	return 0;
}