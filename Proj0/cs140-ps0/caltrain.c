#include "pintos_thread.h"

/* NO BUSY WAITING ANYWHERE */


/* 
   must include the number of open seats for the train, 
   the number of waiting passengers, and the number of
   boarding passengers 

   !!!!!!!!there can only be one lock in this struct!!!!!!!!!
*/
struct station 
{
  /* So here we have e*/

  unsigned int num_open;
  struct condition train_wait_1;

  unsigned int num_waiting;
  struct condition wait_queue;

  unsigned int num_boarding;
  struct condition train_wait_2;   //this condition really just consists of the train

  struct lock boarding_lock;   // the lock used for everything

  
};

/* Don't know why this wasn't included... */
void station_on_board(struct station*);


void
station_init(struct station *station)
{
  station->num_open = 0;
  station->num_waiting = 0;
  station->num_boarding = 0;
  
  cond_init(&station->wait_queue);
  cond_init(&station->train_wait_1);
  cond_init(&station->train_wait_2);

  lock_init(&station->boarding_lock);
}

/*
 
1. The function must not return until the train is satisfactorily loaded 
   (all passengers are in their seats, and either the train is full or *all*
   waiting passengers have boarded). 

2. May assume there is never more than one train in the station at once.

3. May assume any passenger may board any train.

*/
void station_load_train(struct station *station, int count)
{
  // there should be a loop in here first checking if 
  // there are any open seats, and secondly if there are
  // any passengers waiting. 

  /* Perhaps we can lock both passengers and seats at once */
  lock_acquire(&station->boarding_lock);
  station->num_open = count;
  while (station->num_open > 0)
    {
      if (station->num_waiting > 0)
	{
	  /* The key is to update here, since all conditions 
             to continue are checked in *this* function, and the 
             passenger threads may not have updated these variables
             before the condition is checked. If we've reached this 
             point we know all these variables need to be updated anyways. */
	  station->num_open--;
	  station->num_boarding++;
	  station->num_waiting--;
	  
	  /* Finally signal to the passenger thread to wake. */
	  cond_signal(&station->wait_queue, &station->boarding_lock);
	  
	  /* Note that in this case, however, the lock is still held by this thread.
	     If we don't release the lock now the cond_wait will never return. */

	  cond_wait(&station->train_wait_1, &station->boarding_lock);
	}

      else
	{
	  /* still possesses the boarding_lock here */
	  station->num_open = 0;
	  break;
	}
    }

  
  /* If we're here the train still has the boarding lock...
     which we can probably re-use. Remember: one lock may be associated
     with many condition variables. 

     Now I see how this makes sense...for example, in this case we have a 
     sequential use of the lock such that it operates on different 
     conditions. */
  while (station->num_boarding > 0)
    {
      cond_wait(&station->train_wait_2, &station->boarding_lock);
    }
  
  /* If we're here all passengers are in their seats. */
  lock_release(&station->boarding_lock);

  return;
}


/* This code must allow multiple passengers to board simultaneously (it must
   be posible for several passengers to have called station_wait_for_train,
   and for that function to have returned for *each* of the p*/
void station_wait_for_train(struct station *station)
{
  lock_acquire(&station->boarding_lock);
  if (station->num_open == 0)
    {
      station->num_waiting++;
      cond_wait(&station->wait_queue, &station->boarding_lock);
    }

  // NOTE: THE KEY INSIGHT OF THE DAY IS THAT THIS STEP CAN ONLY BE REACHED
  // ONCE THE TRAIN IS WAITING
  cond_signal(&station->train_wait_1, &station->boarding_lock);

  lock_release(&station->boarding_lock);

  /* Only once this code has reached here can the train resume 
     its first while loop execution. */

  return;
}

void
station_on_board(struct station *station)
{
  lock_acquire(&station->boarding_lock);
  station->num_boarding--; 
  
  /* Basically this should only signal the train to wake up once
     the numver of boarding passengers is 0 */
  if (station->num_boarding == 0)
    {
      // if the train isn't ready yet this should do nothing 
      cond_signal(&station->train_wait_2, &station->boarding_lock);
    }

  lock_release(&station->boarding_lock);
}
