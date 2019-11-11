#pragma once

#ifdef _WIN32
#include <windows.h>
#include <winnt.h>
#include <intrin.h>
#endif

typedef struct spinlock_s
{
#ifdef _WIN32
	volatile long lock;
#else
	volatile int lock;
#endif
} spinlock_t;

#define SPINLOCK_INITIALIZER { 0 }

#ifdef _WIN32

static inline void cpu_relax( void ) {
	YieldProcessor( );
}

static inline int spinlock_trylock( spinlock_t *spinlock ) {
	return 0 == InterlockedCompareExchange( &spinlock->lock, 1, 0 );
}

#else
static inline int cmpxchgi( int *ptr, int oldval, int newval ) {
#if defined(__i386__) || defined(__x86_64__)
	int out;
	__asm__ __volatile__( "lock; cmpxchg %2, %1;"
						  : "=a" (out), "+m" (*(volatile int *)ptr)
						  : "r" (newval), "0" (oldval)
						  : "memory" );
	return out;
#elif defined(_AIX) && defined(__xlC__)
	const int out = (*(volatile int *)ptr);
	__compare_and_swap( ptr, &oldval, newval );
	return out;
#elif defined(__MVS__)
	unsigned int op4;
	if( __plo_CSST( ptr, (unsigned int *)&oldval, newval,
		(unsigned int *)ptr, *ptr, &op4 ) )
		return oldval;
	else
		return op4;
#elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
	return atomic_cas_uint( (uint_t *)ptr, (uint_t)oldval, (uint_t)newval );
#else
	return __sync_val_compare_and_swap( ptr, oldval, newval );
#endif
}

static inline void cpu_relax( void ) {
#if defined(__i386__) || defined(__x86_64__)
	__asm__ __volatile__( "rep; nop" );  /* a.k.a. PAUSE */
#endif
}

static inline int spinlock_trylock( spinlock_t *spinlock ) {
	return 0 == cmpxchgi( &spinlock->lock, 0, 1 );
}


#endif

static inline void uv_spinlock_init( spinlock_t *spinlock )
{
	spinlock->lock = 0;
}


static inline void spinlock_lock( spinlock_t *spinlock ) {
	while( !spinlock_trylock( spinlock ) ) cpu_relax( );
}

static inline void spinlock_unlock( spinlock_t *spinlock ) {
	spinlock->lock = 0;
}