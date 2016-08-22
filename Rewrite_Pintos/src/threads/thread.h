/* In these files I rewrite all of the basic pintos code: in particular, below each
   new line of code I will attempt to explain its usage and why it was included in the manner
   that it was. */


#ifndef THREADS_THREAD_H 
#define THREADS_THREAD_H

/* MY COMMENTS:
   The set of above code is called an "include guard." Essentially, it first checks to see
   if anywhere THREADS_THREAD_H is defined: if it's not defined, it replaces it with nothing 
   since otherwise an error will occur. This holds up until the endif statement at the bottom. */


#include <debug.h>
#include <list.h>
#include <stdint.h>

/* MY COMMENTS: 
   different header files to be included later on: we will examine these separately. */

/* States in a thread's life cycle. */
enum thread_status
  {
    THREAD_RUNNING,     /* Running thread. */
    THREAD_READY,       /* Not running but ready to run. */
    THREAD_BLOCKED,     /* Waiting for an event to trigger. */
    THREAD_DYING        /* About to be destroyed. */
  };

/* MY COMMENTS: just defining a variable which will cover states in a thread's life cycle. */


/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* MY COMMENTS: here a type is defined to identify threads: it's really just an integer,
   probably redefined for book-keeping purposes. The TID_ERROR is just used to define the 
   an error occurrence in the thread: type casted of course to a thread identifier type. */

/* Thread Priorities. */
#define PRI_MIN 0                       /* Lowest Priority. */
#define PRI_DEFAULT 31                  /* Default Priority. */
#define PRI_MAX 63                      /* Highest priority. */

/* MY COMMENTS: Don't think it is an accident that the range here can be captured in  6 bits. */

/* A kernel thread or user process. 

   Each thread structure is stored in its own 4 kB page. The 
   thread structure itself sits at the very bottom of the page
   (at offset 0). The rest of the page is reserved for the    
   thread's kernel stack, which grows downward from the top of 
   the page (at offset 0). The rest of the page is reserved for 
   the thread's kernel stack, which grows downward from the top of
   the page (at offset 4 kB). Here's an illustration:
   
        4 kB +---------------------------------+
             |          kernel stack           |
             |                |                |
             |                |                |
             |                V                |
             |         grows downward          |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             +---------------------------------+
             |              magic              |
             |                :                |
             |                :                |
             |               name              |
             |              status             |
        0 kB +---------------------------------+


    The upshot of this is twofold:

        1. Firstly, 'struct thread' must not be allowed to grow
	too big. If it does, then there will not be enough room
	for the kernel stack. Our base 'struct thread' is only a
	few bytes in size. It probably shoudl stay well under 1 kB.

	2. Secondly, kernel stacks must not be allowed to grow too
	large. If a stack overflows, it will corrupt the thread state.
	Thus, kernel functions should not allocate large structures
	or arrays as non-static local variables (so static variables
        are *not* stored on the stack). Use dynamic allocation with 
	malloc() or palloc_get_page() instead. 


    The first symptom of either of these problems will probably be an 
    assertion failure in "thread_current()", which checks that the 'magic'
    member of the running thread's 'struct thread' is set to THREAD_MAGIC 
    (evidently a magic number). Stack overflow will normally change this 
    value, triggering the assertion. */

/*  The 'elem' member has a dual purpose. It can be an element in the run
    queue (thread.c), or it can be an element in a semaphore wait list 
    (synch.c). It can be used these two ways only because they are mutually
    exclusive: only a thread in the ready state is on the run queue, whereas
    only a thread in the blocked state is on a semaphore wait list.
*/

struct thread
{

  /* Owned by thread.c */
  tid_t tid;                          /* Thread identifier. */
  enum thread_status status;          /* Thread state. */
  char name[16];                      /* Name (for debugging purposes). */
  uint8_t *stack;                     /* unsigned integer with exactly 8 bits. pointer to 
				         the stack. */
  int priority;                       /* Priority of the thread as previously described:
       				         6 bit range. */

  /* Shared between thread.c and synch.c */
  struct list_elem allelem;           /* List element. */


  /* MY COMMENTS: realize here macro conditionals can be located in the middle of code*/
#ifdef USERPROG
  /* Owned by userprog/process.c */
  uint32_t *pagedir;                  /* Page directory. */
#endif

  /* Owned by thread.c */
  unsigned magic;                     /* detects stack overflow. */

  };

#endif /* threads/thread.h */


/* If false (default), use round-robin scheduler. 
   If true, use multi-level feedback queue scheduler.
   Conrolled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs; 

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef voidthread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);
