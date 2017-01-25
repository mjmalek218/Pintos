#include "pintos_thread.h"

/* We attempt to place this problem in the consumer/producer paradigm, so as 
   to organize the solution according to guidelines which are known to produce
   a correct solution.

   First Half: Passengers boarding train

   In this half of the problem, the passenger threads are loaded into a queue, 
   constituting the buffer. They "produce" themselves. The arriving train thread,
   which we can assume is by itself in the station (thus only one consumer thread),
   "consumes" the passengers. Remember to reset the count once the train has departed.
  
   Second Half: Passengers seating

   Just a condition the train needs to wait for 

*/

/* We define four condition variables here. The first two are self-explanatory.
   The second two correspond to single-element queues of the train, which passengers
   at different stages queue to alert it of possible completion of the current 
   process (boarding or seating).

   Notice we choose to name cv's based upon what the queues themselves are made of,
   not the condition upon which they are signaled

   Also notice there is no queue of trains: this is due to the design of the 
   problem. Specifically, the train is allowed to leave as soon as it arrives,
   if no passengers are waiting. Thus it only needs to check once before leaving. 

   Another problem here is that there are *two* conditions for a train leaving
   the station: one, all waiting passengers have been loaded, and two, that there
   is no more space aboard the train. The train needs to be alerted as soon as
   one of these conditions are met. The solution we will provide is to have
   both conditions checked simultaneously, and to have one condition variable
   encompassing both of these conditions. 

*/


/* variables names are designed to be self-explanatory */
struct station {
  int num_at_station_waiting;   

  int num_on_train_waiting; 

  int num_seats_available; 

  int num_waiting_to_sit;

  struct condition at_station_waiting, on_train_waiting, train_1, train_2;

  /* limited to one lock by problem specification, so we use this lock for all
     station variables. Perhaps overkill but guarantees synchronization */
  struct lock lock;   

};

void station_init(struct station *station)
{
  /* Initialize the lock and conditions, initialize all ints to 0 */
  cond_init(&station->at_station_waiting);
  cond_init(&station->on_train_waiting);
  cond_init(&station->train_1);
  cond_init(&station->train_2);
  lock_init(&station->lock);

  lock_acquire(&station->lock);
  station->num_at_station_waiting = 0;
  station->num_on_train_waiting = 0;
  station->num_seats_available = 0;
  station->num_waiting_to_sit = 0;
  lock_release(&station->lock);
}

/* This should signal to all passenger threads waiting at the 
   station that a train has arrived. */
void station_load_train(struct station *station, int count)
{
  lock_acquire(&station->lock);

  if (count == 0)
    {
      lock_release(&station->lock);
      return;
    }

  station->num_seats_available = count;

  while(station->num_at_station_waiting > 0 && station->num_seats_available > 0)
    {
      cond_broadcast(&station->at_station_waiting, &station->lock);
      cond_wait(&station->train_1, &station->lock);
    } 

  /* At this point we need to reset the number of seats available, 
     since this train is closed and departing */
  station->num_seats_available = 0;


  while(station->num_waiting_to_sit > 0)
    {
      cond_wait(&station->train_2, &station->lock);
    }



  lock_release(&station->lock);
  
  return;
}

/* called as soon as a passenger arrives, must
   only return once the passenger is on board. */
void station_wait_for_train(struct station *station)
{
  lock_acquire(&station->lock);
 
  /* Since the train will continuously broadcast until it is 
     full or no passengers are waiting, we can just wait immediately */
  station->num_at_station_waiting++;

  while (station->num_seats_available <= 0)
    {
      cond_wait(&station->at_station_waiting, &station->lock);
    }

  station->num_at_station_waiting--;
  station->num_seats_available--;
  station->num_waiting_to_sit++;

  cond_signal(&station->train_1, &station->lock);

  lock_release(&station->lock);

  return;

}

/* called when a passenger boards, must only return
   once he/she has seated */
void station_on_board(struct station *station)
{
  lock_acquire(&station->lock);
  station->num_waiting_to_sit--;
  
  cond_signal(&station->train_2, &station->lock);
  
  lock_release(&station->lock);

  return;
}
