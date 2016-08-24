
/* Can assume only one train in the station at a time so no condition 
   for trains necessary */
struct station{

  bool is_train;
  unsigned int num_open;
  unsigned int pass_waiting;
  lock* count_lock; 
  lock* 

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
void station_load_train(struct station* station, int count)
{
  if (count <= 0)
    return;

  station->is_train = true;
  station->num_open = count;
  
  /* Need a way to signal to the function that it is completely boarded */
  while (station->num_open && station->pass_waiting)
    {
      lock_acquire(station->count_lock);
      /* singal to next-in-line thread that empty space is available */
      cond_signal(station->pass_waiting, station->count_lock);  
    }

  return;
}

/* must NOT return until a train is in the station and it has room to board */
void station_wait_for_train(struct station *station)
{
  cond_wait(station->pass_waiting, station->count_lock);
  
  /* Since the lock has been acquired...let the passenger board */
  station->num_open--; 

  /* then release the lock on the count */
  lock_release(station->count_lock);
}

// TODO: Once the passenger is seated, it calls the following
// function to let it know that it's on board
void station_on_board(struct station *station)
{

  /* Probably should lock this as well... need to think about it */
  station->pass_waiting--;


}
