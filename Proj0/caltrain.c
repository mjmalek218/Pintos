
/* Can assume only one train in the station at a time so no condition 
   for trains necessary */
struct station{

  bool is_train;
  unsigned int num_open;
  unsigned int pass_waiting;
  lock count_lock; 

  /* ok...so now it's apparent to me that a condition variable 
     could be necessary for waiting for a train with 
     available spots to enter the station.  */
  condition pass_waiting;
};

/* initializes the lock, if there's a train there, number waiting, etc... */
void station_init(struct station* station)
{
  station->is_train = false;
  station->pass_waiting = station->num_open = 0;
  lock_init (station->count_lock);
  cond_init (station->pass_waiting);
}

/* count indicates how many seats are available on the train. 
   the function must not return until the train is satisfactorily loaded--
   i.e. all passengers are in their seats, and either the train is full or 
   all waiting passengers have boarded. */
void station_load_train(struct station *station, int count)
{
  if (count <= 0)
    return;

  station->is_train = true;
  station->num_open = count;
  
  /* broadcast to all waiting threads empty space is available */
  cond_broadcast (station->pass_waiting, station->count_lock);  

  /* Need a way to signal to the function that it is completely boarded*/

  return;
}

/* must NOT return until a train is in the station and it has room to board */
void station_wait_for_train(struct station *station)
{
  cond_wait(station->pass_waiting, station->count_lock);
}

// TODO: Once the passenger is seated, it calls the following
// function to let it know that it's on board
void station_on_board(struct station *station)
{
  


}
