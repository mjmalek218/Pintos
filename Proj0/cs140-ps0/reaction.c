#include "pintos_thread.h"

// Forward declaration. This function is implemented in reaction-runner.c,
// but you needn't care what it does. Just be sure it's called when
// appropriate within reaction_o()/reaction_h().
void make_water();

/* Again we attempt to frame our code in terms of the producer/consumer
   paradigm. The problem here is that, from the outset, it seems as though
   all the threads are simultaneously producers/consumers, as hydrogen
   and oxygen are producing/consuming themselves to create water. 

   Our approach will be to frame the reaction in a way un-natural to the
   chemistry, but natural to our producer/consumer paradigm. The oxygen
   threads will be the "consumer" threads and the *only* threads
   calling "make water", and the hydrogen's will be the "producer" 
   threads, producing themselves, ofc. There will be only one count,
   tracking the number of hydrogen atoms.

   Whenever an H atom is created, it will increment the total number
   of these atoms and broadcast to oxygen atoms to check for satisfactory
   conditions (for the reaction to occur: mianly that there are at least
   2 H atoms). It will then sleep until it is signaled by an oxygen atom 
   that it is ready for the reaction to occur 

   Whenver an 
 
*/


struct reaction 
{
  int num_h;

  struct condition waiting_h, waiting_o; 

  /* all purpose lock. problem specifies we can have at most one */
  struct lock lock;

};

void reaction_init(struct reaction *reaction)
{
  reaction->num_h = 0;
  lock_init(&reaction->lock);
  cond_init(&reaction->waiting_h);
  cond_init(&reaction->waiting_o);
}

void reaction_h(struct reaction *reaction)
{
  lock_acquire(&reaction->lock);

  reaction->num_h++;

  cond_broadcast(&reaction->waiting_o, &reaction->lock);

  cond_wait(&reaction->waiting_h, &reaction->lock);

  lock_release(&reaction->lock);

  return;
}

void reaction_o(struct reaction *reaction)
{
  lock_acquire(&reaction->lock);

  while (reaction->num_h < 2)
    {
      cond_wait(&reaction->waiting_o, &reaction->lock);
    }

  /* Signal *exactly* twice and decrement here to ensure the operation
     is atomic */

  reaction->num_h-= 2;
  cond_signal(&reaction->waiting_h, &reaction->lock);
  cond_signal(&reaction->waiting_h, &reaction->lock);

  /* One problem here is that we *need* to ensure the atomicity of this operation. 
     Before we started decrementing the number of h inside this function, 
     another O atom may have bene created in between and signaled more H atoms
     to wake up then we wuld have wanted. */

  lock_release(&reaction->lock);

  make_water();

  return;

}
