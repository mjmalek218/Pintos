#ifndef THREADS_SYNCH_H
#define THREADS_SYNCH_H

#include <list.h>
#include <stdbool.h>

/* A counting semaphore. */
struct semaphore 
  {
    unsigned value;             /* Current value. */
    struct list waiters;        /* List of waiting threads. */
  };

void sema_init (struct semaphore *, unsigned value);
void sema_down (struct semaphore *);
bool sema_try_down (struct semaphore *);
void sema_up (struct semaphore *);
void sema_self_test (void);

/* Lock. */
struct lock 
  {
    /* Thread holding lock. Originally for debugging, but with priority donation...
       necessary for updating donated_priority field once a lock is released. */
    struct thread* holder;      

    /* 
       for the list of locks held by every thread: because any lock could potentially
       be a part of this list, defining a new struct ist worth it imo. 
       We can conserve some space here, by designing the function such that the holder
       field is NULL iff the lock is not held by any thread. Thus the value field is
       removed. 
    */
    struct list_elem* lock_list_elem;
    
    
    /**********  BEGIN priority donation changes ******/

    /* Before a semaphore was included here. While it may seem superfluous 
       to have deleted the semaphore and then just included the same fields
       it had previously, due to the differences that priority donation will
       necessitate later on, none of the semaphore methods could be recycled
       and thus just simply wasn't clean to keep. 
    */
    struct list waiters; 

    /******** END priority donation changes **********/
    
  };


void lock_init (struct lock *);
void lock_acquire (struct lock *);
bool lock_try_acquire (struct lock *);
void lock_release (struct lock *);
bool lock_held_by_current_thread (const struct lock *);

/* Condition variable. */
struct condition 
  {
    struct list waiters;        /* List of waiting threads. */
  };

void cond_init (struct condition *);
void cond_wait (struct condition *, struct lock *);
void cond_signal (struct condition *, struct lock *);
void cond_broadcast (struct condition *, struct lock *);

/* Optimization barrier.

   The compiler will not reorder operations across an
   optimization barrier.  See "Optimization Barriers" in the
   reference guide for more information.*/
#define barrier() asm volatile ("" : : : "memory")

#endif /* threads/synch.h */
