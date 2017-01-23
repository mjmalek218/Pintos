#include "pintos_thread.h"
#include <stdio.h>

/* 

   OVERALL BULLET POINTS IN THE ASSIGNMENT

   --the code must invoke "make_water" exactly once for ever two H
     and one O atoms that call reaction_h and reaction o, and *only*
     when these calls are active

   --only one lock allowed

   --no busy waiting

*/



// Forward declaration. This function is implemented in reaction-runner.c,
// but you needn't care what it does. Just be sure it's called when
// appropriate within reaction_o()/reaction_h().
void make_water();

struct reaction {

  unsigned int num_H;
  unsigned int num_O;

  struct lock reaction_lock;

  /* conditions for extra H and O that are waiting due to an imbalanced surplus */
  struct condition H_wait;
  struct condition O_wait;

};


void reaction_init(struct reaction *reaction)
{
  reaction->num_H = 0;
  reaction->num_O = 0;

  lock_init(&reaction->reaction_lock);

  cond_init(&reaction->H_wait);
  cond_init(&reaction->O_wait);

}

/* Invoked by an H atom when it is ready to react */
void reaction_h(struct reaction *reaction)
{
  lock_acquire(&reaction->reaction_lock);

  /* increment *immediately* so that we can assess the current state of the reactions
     with updated information */
  reaction->num_H++;

  //DELETE THIS DELETE THIS
  printf("REACHED H: 1 \n");  fflush(stdout);

  /* If there are enough H, then wake up the O thread and only allow it to signal
     *once* this hydrogen element is waiting by waiting until cond_wait to release the 
     lock*/
  if (reaction->num_H >= 2 && reaction->num_O >= 1)
    {
      cond_signal(&reaction->O_wait, &reaction->reaction_lock);
    }

 
  /* wait for the signal from oxygen */
  cond_wait(&reaction->H_wait, &reaction->reaction_lock);

  //DELETE THIS DELETE THIS
  printf("REACHED H: 2 \n");  fflush(stdout);

  /* lock has been re-acquired */
  reaction->num_H--;
  lock_release(&reaction->reaction_lock);

  return;
}

/* PROBLEM PROBLEM PROBLEM: THIS CODE ASSUMES THAT HYDROGEN HAPPENS *BEFORE* OXYGEN

   FIXFIXFIX

 */


void reaction_o(struct reaction *reaction)
{
  lock_acquire(&reaction->reaction_lock);

  reaction->num_O++;

  // We only reach here if there is positive oxygen count so no need to check
  // for that in the condition
  while (reaction->num_H < 2)
    {
      //DELETE THIS DELETE THIS
      printf("REACHED OXYGEN LOOP\n");  fflush(stdout);

      cond_wait(&reaction->O_wait, &reaction->reaction_lock);
    }

  /* lock has been re-acquired */
  reaction->num_O--;


  cond_signal(&reaction->H_wait, &reaction->reaction_lock);
  cond_signal(&reaction->H_wait, &reaction->reaction_lock);


  /* Calling "make-water" in this file makes much more sense so we don't get 
     multiple calls */

  printf("Make Water is about to be called.\n"); fflush(stdout);
  make_water();

  /* release after the call to make sure that the two hydrogen atoms return 
     after the call as well. */
  lock_release(&reaction->reaction_lock);

  return;

}
