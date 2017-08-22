/* NEED TO WRITE TEST SUITE FOR THIS GARBAGE TODO TODO TODO
******************************************************************************************

D SADASDSADSADSADSADSADSADSADASDASDASDASD */



/* Contains operations for fixed_point arithmetic. 

   Not can probably re-use this file for other projects.

   This is as recommended in section B.6 of the Pintos Directory

   Notation (given by Pintos documentation): 

   -- x and y are fixed-point numbers

   -- n is an integer

   -- fixed-pointer numbers are in signed p.q format where p+q = 31

   NOTE: Arithmetic operators are used in lieu of the simpler shift 
         operators due to unpredictable behavior on negative #'s

	 Actually...it seems as though 2's complement preserves 
	 even negative numbers under left-shifting unless there
         is overflow

   WARNING: This implementation assumes arithmetic right shifting for the underlying 
            implementation. The C compiler could very well implement logical shifting:
            nothing is required by any standard 

 */ 

#include <inttypes.h>

/* Define the desired level of fractional precision...for now we use the example's 14
   fractional bits.  */
#define FRAC_P 14

/* note we use signed here. */
typedef int32_t fp;

inline min(fp x, fp y)
{
  return (x < y) ? x : y;
}

inline max(fp x, fp y)
{
  return (x < y) ? y : x;
}

inline fp int_to_fp(int32_t);
inline int32_t fp_to_int(int32_t);
inline fp add_fp(fp, fp);
inline fp mult_fp(fp, fp);
inline fp div_fp(fp, fp);



inline fp int_to_fp(int32_t n)
{
  return n << FRAC_P;
}

/* This attempts to maximize precision by rounding to the nearest, at
   the expensive of extra computational cost. We don't implement the 
   simpler version of always rounding towards 0...for now */
inline int32_t fp_to_int(int32_t x)
{
  if (x >= 0)
    return (x + (1 << (FRAC_P - 1))) >> FRAC_P;

  else 
    return (x - (1 << (FRAC_P - 1))) >> FRAC_P;
}

/* To subtract, multiply values by -1 using the 'mult_fp' functon then add. */
inline fp add_fp(fp x, fp y)
{
  return x + y;
}

/* The simplest way to implement this is to employ 64 bit integers.
   Even 32 bit machines allow for these extensions, as demonstrated
   by the casting we use. */
inline fp mult_fp(fp x, fp y)
{
  return (fp) ((( (int64_t) x) * y) >> FRAC_P);  
}

inline fp div_fp(fp x, fp y)
{
  return (fp) ((( (int64_t) x) << FRAC_P) / y);
}


