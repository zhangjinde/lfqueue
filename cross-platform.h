#ifndef __CROSS_PLATFORM_H__
#define __CROSS_PLATFORM_H__
#ifdef __KERNEL__
	#define malloc(x) kmalloc(x, GFP_KERNEL )
	#define free kfree
	#define calloc(x,y) kmalloc(x*y, GFP_KERNEL | __GFP_ZERO )
	#include<linux/string.h>
	#include <sys/stdbool.h>
#else
	#include <stdlib.h>
	#include <string.h>
	#include <stdbool.h>
#endif

#ifndef asm
	#define asm __asm
#endif

#ifndef __GCC__
	#include <windows.h>
	#include <stdatomic.h>
	#define CAS atomic_compare_exchange_weak
	#define ATOMIC_ADD atomic_fetch_add
	#define ATOMIC_SUB atomic_fetch_sub
	#define ATOMIC_SET atomic_store
	#define mb() atomic_thread_fence(memory_order_acq_rel)
	#define smb() atomic_thread_fence(memory_order_release)
	#define lmb() atomic_thread_fence(memory_order_acquire)
#else
	#define CAS __sync_bool_compare_and_swap
	#define ATOMIC_ADD __sync_add_and_fetch
	#define ATOMIC_SUB __sync_sub_and_fetch
	#define ATOMIC_SET __sync_lock_test_and_set
	#define mb __sync_synchronize
	#define smb() asm volatile( "sfence" )
	#define lmb() asm volatile( "lfence" )
#endif




#endif

