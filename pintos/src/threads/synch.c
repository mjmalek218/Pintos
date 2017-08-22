/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"


/*********** BEGIN PRIORITY DONATION CHANGES ***************/

/* A few helper functions not required outside of synchronization */
void percolate_priority(int base_priority, struct lock* lock);
void update_priority();

/*********** END PRIORITY DONATION CHANGES ***************/


/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
     decrement it.

   - up or "V": increment the value (and wake up one waiting
     thread, if any). */
void
sema_init (struct semaphore *sema, unsigned value) 
{
  ASSERT (sema != NULL);

  sema->value = value;
  list_init (&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. */
void
sema_down (struct semaphore *sema) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  while (sema->value == 0) 
    {
      list_push_back (&sema->waiters, &thread_current ()->elem);
      thread_block ();
    }
  sema->value--;
  intr_set_level (old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool
sema_try_down (struct semaphore *sema) 
{
  enum intr_level old_level;
  bool success;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (sema->value > 0) 
    {
      sema->value--;
      success = true; 
    }
  
  else
    success = false;
  intr_set_level (old_level);

  return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
void
sema_up (struct semaphore *sema) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);

  old_level = intr_disable ();

  /******************* BEGIN 2.2.3 CHANGES **************/
  
  if (!list_empty (&sema->waiters))
    {
      /* This is where the change should be made... rather than 
         just wake up the first thread at the front of the list, wake
         up the one with the highest priority. Since lock relies
         on semaphores for its base functionality, we kill 2 birds with
         one stone here. NOTE: cond also used to, but we removed that
         for reasons discussed later. */
      struct thread* thread_to_choose = highest_ready(&sema->waiters);

      /* Remove from the waiters list and unblock it to access the resource. */
      list_remove(&thread_to_choose->elem);
      thread_unblock(thread_to_choose);
      
    }

  /******************* END 2.2.3 CHANGES ******************/
  
  sema->value++;
  intr_set_level (old_level);
}


  
static void sema_test_helper (void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void
sema_self_test (void) 
{
  struct semaphore sema[2];
  int i;

  printf ("Testing semaphores...");
  sema_init (&sema[0], 0);
  sema_init (&sema[1], 0);
  thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++) 
    {
      sema_up (&sema[0]);
      sema_down (&sema[1]);
    }
  printf ("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper (void *sema_) 
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++) 
    {
      sema_down (&sema[0]);
      sema_up (&sema[1]);
    }
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   The way we determine if lock is held or not is by the "holder"
   field: if NULL, the lock is NOT held. Otherwise, it is held. 
 */
void
lock_init (struct lock *lock)
{
  ASSERT (lock != NULL);
  lock->holder = NULL;
  lock->lock_list_elem = NULL
  list_init(&lock->waiters);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
lock_acquire (struct lock *lock)
{
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (!lock_held_by_current_thread (lock));

  /******** BEGIN PRIORITY DONATION ADDITIONS ************/

  /* Begin section where sema_down(&lock->semaphore) once was. The following
     is just directly transplanted code. */
  enum intr_level old_level;

  ASSERT (!intr_context ());

  old_level = intr_disable ();  // important, can't have timer interrupts occurring here 

      /* TO THINK ABOUT: do we really need to call percolate_priority every time???
         probably should just set an if statement first and then call the while, could
         save a lot of time...Once the priority has percolated once lock_release should
         maintain it 
	 
	 NEED TO COME BACK AND THINK ABOUT THIS.
	 ALSO: how to ensure that the highest priority thread waiting on a lock is
	 woken up first? 

	 ANSWER: need to update this in lock_release. in the vanilla priority donation
	 implementation everything was changed in one fell swoop just by altering the semaphore 
	 code...but really it should be in lock_release that the thread with the highest
	 merged_priority should be woken up first. 

	 // TODO TODO COME BACK TO LOCK ACQUIRE NOT DONE YET

	 Yes...calling once should suffice, since if another thread
	 takes the lock before this one, it must have been higher 
	 priority to do so. 
*/
  
  /* this preliminary call is just to make sure the thread's */
  if (lock->holder != NULL) 
    {
      thread_current ()->waiting_on = lock;  
      percolate_priority(thread_current->merged_priority, lock);
      list_push_back (&lock->waiters, &thread_current ()->elem);
      thread_block ();
    }

  while (lock->holder != NULL) 
    {
      thread_current ()->waiting_on = lock;
      list_push_back (&lock->waiters, &thread_current ()->elem);
      thread_block ();
    }

  
  /* Now we update the coordination mechanisms between the lock and the thread:

     1. the lock's holder field should be set to the current thread
     2. the thread is no longer waiting on a lock, so that is set to NULL
     3. the thread now holds the lock, so its list of held locks is updated 
*/
  lock->holder = thread_current ();
  thread_current ()->waiting_on = NULL;
  list_push_back(&thread_current ()->locks_held, lock->lock_list_elem);

  intr_set_level (old_level);

  /* End section where sema_down(&lock->semaphore) once was */

  
  /******* END PRIORITY DONATION ADDITIONS ****************/
  

  lock->holder = thread_current ();
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool
lock_try_acquire (struct lock *lock)
{
  bool success;

  ASSERT (lock != NULL);
  ASSERT (!lock_held_by_current_thread (lock));

  success = sema_try_down (&lock->semaphore);
  if (success)
    lock->holder = thread_current ();
  return success;
}

/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */


/* TODO TODO:

  What needs to happen here is the following:

  1. Needs to remove the lock from the lock_list

  2. Needs to *update* its own donated priority, to reflect
     the highest donated priority of all other locks it holds. 


    To expand on 2, Say, for example, in a nested chain 

  3. Needs to go in and *unblock* the highest ready thread
     and give control to that thread...again another reason to
     re-write all of the lock code: this has to take into account the
     donated priorities of all waiting threads, which as of rn (and probably
     forever) is not something that applies to "upping" a semaphore. 

  END TODO TODO
*/

/* CONTINUE MY COMMENTS:

   An earlier thought I had was that, when lock_release is called, the
   thread should also update priorities of threads it is waiting on...since
   it's donated priotiy field will be altered as a result 

   Completely unecessary now that I think about it: a thread cannot just go
   and *wait* on a lock and then do other things simultaneously...i.e. call lock_release
   on another lock. THe point is...the thread *would not* be calling lock_release if
   it was also waiting on another lock: if it were, that is *all* it would be doing
   and would be stuck in a while loop.    

*/

void
lock_release (struct lock *lock) 
{
  ASSERT (lock != NULL);
  ASSERT (lock_held_by_current_thread (lock));

  /* update holder */
  lock->holder = NULL;

  /* Remove the lock from the list of held locks */
  list_remove(lock->lock_list_elem);
  
  /* update the thread's merged_priority */
  update_priority();
  
  /* wake up highest ready thread */
  thread_unblock(highest_ready(&lock->waiters));

}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool lock_held_by_current_thread (const struct lock *lock) 
{
  ASSERT (lock != NULL);

  return lock->holder == thread_current ();
}

/* One semaphore in a list. */
struct semaphore_elem 
  {
    struct list_elem elem;              /* List element. */
    struct semaphore semaphore;         /* This semaphore. */
  };

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void
cond_init (struct condition *cond)
{
  ASSERT (cond != NULL);

  list_init (&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
cond_wait (struct condition *cond, struct lock *lock) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  enum intr_level old_level;
  
  /************** VANILLA PRIORITY SCHEDULING CHANGES  ***************/

  /* I don't understand why we need the waiters to be a list of semaphore
     elems...just block/unblock directly with the synchronization list_elem
     provided in the original thread struct...

     OK I see now. If we just block directly, we risk unblocking the thread
     with cond_signal *prior* to the thread blocking...and then when the 
     thread does block, it blocks forever. 

     So then what's the solution? 

     1. disable interrupts while the lock is released and then the thread
       blocked. 

     2. add a semaphore to each thread's struct. 

     3. add the thread priority to the sempahore_elem 

     2 and 3 are bad imo, 3 especially since it will make priority donation
     more laborious than it already is (every time a threads priority is updated
     we would soimehow have to check every condition variable it might be in,
     and find it somehow in the list and update the priority accordinfly: quite
     bad). I would rather just link to the thread directly. 

     OK semaphores disable interrupts anyway, so just do that here manually...
     basically no time is lost whereas adding memory to each thread's struct
     is a much more significant change imo. 

     CHANGE CHANGE !!! MOVE THIS COMMENT TO THE DESIGN DOC TO LARGE HERE

     wait...but there's another problem now: what if the scheduler just schedules
     the thread back again?

     ANSWER: it WONT because the scheduler can only schedule READY threads. So this
     seems like a sound, simplifying solution. 
 */  
  list_push_back (&cond->waiters, &thread_current()->elem);

  /* The following two operations are what need to be atomic. Calling thread_block() 
     requires interrupts to be disabled anyways... */
  old_level = intr_disable();
  lock_release (lock);
  thread_block();
  intr_set_level (old_level);


  // OK NOW COME BACK TO THIS FUNCTION NEXT AND ALTER THE REMAINING COND FUNCITONALITY
  // APPROPRIATELY
  
  /************** END VANILLA PRIORITY SCHEDULING CHANGES  ***************/
  
  
  lock_acquire (lock);
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_signal (struct condition *cond, struct lock *lock UNUSED) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  /************** VANILLA PRIORITY SCHEDULING CHANGES  ***************/

  
  if (!list_empty (&cond->waiters))
    {
      /* choose the highest priority thread waiting on the condition, 
         remove it from the waiting list, and unblock. */
      thread_to_wake = highest_ready ();
      list_remove

    }

  
  /************** END VANILLA PRIORITY SCHEDULING CHANGES  ***************/
  
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_broadcast (struct condition *cond, struct lock *lock) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context());
  ASSERT (lock_held_by_current_thread (lock));

  /* TODO: should wakeup threads in the order of highest priority to 
     lowest. */

  /************************ Vanilla Priority Project CHANGES  ********************/
  
  while (!list_empty (&cond->waiters))
  {
    cond_signal (cond, lock);
  }


    /********************** END Vanilla Priority Project CHANGES  ****************/
    
}



/****************** BEGIN PRIORITY DONATION CHANGES ********************/


/* This recursive function is called in lock_acquire() to modularize the chaining 
   functionality. It performs the following steps:  

   1. checks to see if the thread holding the lock has a higher merged_priority 
      than the inputted thread's

   2. If yes, returns. 

   3. If no, 

      i. upgrades the thread holding the lock's merged priority

      ii. checks to see if the holder is itself waiting on a lock

          a. if yes, calls itself recursively (with the *first* priority)

	  b. if no, returns. 


   Interrupts must be disabled when this function is called, to ensure
   the atomicity of the priorities updating. 
*/
void percolate_priority(int base_priority, struct lock* lock)
{
  struct thread* holder = lock->holder;

  /* catch the case where there's nothing to do because the 
     holder has an equal or higher priority.  */
  if (holder->merged_priority >= base_priority)
    {
      return;
    }

  else
    {
      holder->merged_priority = base_priority;

      /* Then perform the recursive call if necessary. */
      if (holder->waiting_on != NULL)
	{
	  percolate_priority(base_priority, holder->waiting_on);
	}
    }  
}

/* 
   lock_release's analogous helper function: rather than moving forward
   in the chain as percolate_priority does for lock_acquire, this function
   goes one step back, analyzing the remaining donating priority *after*
   a lock has been released. 

   handles the case of a NULL lock list as well. 
*/
void update_priority()
{
  struct list_elem* elem;
  struct list* lock_list = thread_current()->locks_held;
  struct lock* curr_lock;
  
  int running_max = thread_current()->native_priority;

  if (lock_list == NULL)
    {
      thread_current()->merged_priority = running_max;
      return;
    }
  
  for (elem = list_begin(lock_list); elem != list_end(lock_list);
       elem = list_next(lock_list))
    {
      /* Just a reminder: the way we are able to take advantage
         of a name "struct list", an inserted *type* (which is 
         normally quite an odd thing to do) is via the pre-processor 
         and defining list_entry as a macro function: then the pre-processor
         can just fill in variable types, which is normally not possible) */
      curr_lock = list_entry (elem, struct list, lock_list_elem);

      /* should be the running max of the merged priorities */
      if (curr_lock->holder->merged_priority > running_max)
	{
	  running_max = curr_lock->holder->merged_priority;
	}      
      
    }

  thread_current()->merged_priority = running_max;
  return; 
}


/****************** END PRIORITY DONATION CHANGES ********************/
