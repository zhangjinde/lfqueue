#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if ( defined (__WIN32__) || defined(_WIN32) )
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include "lfq.h"

#ifndef MAX_PRODUCER
#define MAX_PRODUCER 100
#endif
#ifndef MAX_CONSUMER
#define MAX_CONSUMER 10
#endif

volatile uint64_t cn_added = 0;
volatile uint64_t cn_deled = 0;

volatile uint64_t cn_t = 0;
volatile int cn_producer = 0;

struct user_data{
	long data;
};

#if ( defined (__WIN32__) || defined(_WIN32) )
DWORD WINAPI 
#else
void * 
#endif
addq( void * data ) {
	struct lfq_ctx * ctx = data;
	struct user_data * p=(struct user_data *)0xff;
	long i;
	int ret = 0;
	for ( i = 0 ; i < 500000 ; i++) {
		p = malloc(sizeof(struct user_data));
		p->data=i;
		if ( ( ret = lfq_enqueue(ctx,p) ) != 0 ) {
			printf("lfq_enqueue failed, reason:%s", strerror(-ret));
			__sync_sub_and_fetch(&cn_producer, 1);
			return 0;
		}
		__sync_add_and_fetch(&cn_added, 1);
	}
	__sync_sub_and_fetch(&cn_producer, 1);
	printf("Producer thread exited! Still %d running...\n",cn_producer);
	return 0;
}

#if ( defined (__WIN32__) || defined(_WIN32) )
DWORD WINAPI 
#else
void * 
#endif
delq( void * data ) {
	struct lfq_ctx * ctx = data;
	struct user_data * p;
	int tid = __sync_fetch_and_add(&cn_t, 1);
	
	
	while(ctx->count || cn_producer) {
		p = lfq_dequeue_tid(ctx, tid);
		if (p) {
			free(p);
			__sync_add_and_fetch(&cn_deled, 1);			
		} else
#if ( defined (__WIN32__) || defined(_WIN32) )
			SwitchToThread();
#else
			pthread_yield(); // queue is empty, release CPU slice
#endif
		sleep(0);
	}

	printf("Consumer thread exited %d\n",cn_producer);
	return 0;
}

int main() {
	struct lfq_ctx ctx;
	int i=0;
	lfq_init(&ctx);
#if ( defined (__WIN32__) || defined(_WIN32) )
	HANDLE thread_d[MAX_CONSUMER];
	HANDLE thread_a[MAX_PRODUCER];
#else
	pthread_t thread_d[MAX_CONSUMER];
	pthread_t thread_a[MAX_PRODUCER];
#endif
	
	__sync_add_and_fetch(&cn_producer, 1);
	for ( i = 0 ; i < MAX_CONSUMER ; i++ )
#if ( defined (__WIN32__) || defined(_WIN32) )
		thread_d[i] = CreateThread(NULL, 0, delq, &ctx, 0, 0); 
#else
		pthread_create(&thread_d[i], NULL , delq , (void*) &ctx);
#endif
	
	for ( i = 0 ; i < MAX_PRODUCER ; i++ ){
		__sync_add_and_fetch(&cn_producer, 1);
#if ( defined (__WIN32__) || defined(_WIN32) )
		thread_a[i] = CreateThread(NULL, 0, addq, &ctx, 0, 0); 
#else
		pthread_create(&thread_a[i], NULL , addq , (void*) &ctx);
#endif
	}
	
	__sync_sub_and_fetch(&cn_producer, 1);
	
#if ( defined (__WIN32__) || defined(_WIN32) )
	WaitForMultipleObjects(MAX_PRODUCER, thread_a, 1, INFINITE);
	WaitForMultipleObjects(MAX_CONSUMER, thread_d, 1, INFINITE);
#else
	for ( i = 0 ; i < MAX_PRODUCER ; i++ )
		pthread_join(thread_a[i], NULL);
	
	for ( i = 0 ; i < MAX_CONSUMER ; i++ )
		pthread_join(thread_d[i], NULL);
#endif
	
	printf("Total push %"PRId64" elements, pop %"PRId64" elements.\n", cn_added, cn_deled );
	if ( cn_added == cn_deled )
		printf("Test PASS!!\n");
	else
		printf("Test Failed!!\n");
	lfq_clean(&ctx);
	return (cn_added != cn_deled);
}
