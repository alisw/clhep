// $Id: Ranlux64Engine.cc,v 1.4 2003/08/13 20:00:12 garren Exp $
// -*- C++ -*-
//
// -----------------------------------------------------------------------
//                             HEP Random
//                       --- Ranlux64Engine ---
//                      class implementation file
// -----------------------------------------------------------------------
// A double-precision implementation of the RanluxEngine generator as 
// decsribed by the notes of the original ranlux author (Martin Luscher)
//
// See the note by Martin Luscher, December 1997, entitiled
// Double-precision implementation of the random number generator ranlux
//
// =======================================================================
// Ken Smith      - Initial draft: 14th Jul 1998
//                - Removed pow() from flat method 14th Jul 1998
//                - Added conversion operators:  6th Aug 1998
//
// Mark Fischler  The following were modified mostly to make the routine
//		  exactly match the Luscher algorithm in generating 48-bit
//		  randoms:
// 9/9/98	  - Substantial changes in what used to be flat() to match
//		    algorithm in Luscher's ranlxd.c
//		  - Added update() method for 12 numbers, making flat() trivial
//		  - Added advance() method to hold the unrolled loop for update
//		  - Distinction between three forms of seeding such that it
//		    is impossible to get same sequence from different forms -
//		    done by discarding some fraction of one macro cycle which
//		    is different for the three cases
//		  - Change the misnomer "seed_table" to the more accurate 
//		    "randoms"
//		  - Removed the no longer needed count12, i_lag, j_lag, etc.
//		  - Corrected seed procedure which had been filling bits past
//		    2^-48.  This actually was very bad, invalidating the
//		    number theory behind the proof that ranlxd is good.
//		  - Addition of 2**(-49) to generated number to prevent zero 
//		    from being returned; this does not affect the sequence 
//		    itself.
//		  - Corrected ecu seeding, which had been supplying only 
//		    numbers less than 1/2.  This is probably moot.
// 9/15/98	  - Modified use of the various exponents of 2
//                  to avoid per-instance space overhead.  Note that these
//		    are initialized in setSeed, which EVERY constructor
//		    must invoke.
// J. Marraffino  - Remove dependence on hepString class  13 May 1999
//
// =======================================================================

#include "CLHEP/Random/defs.h"
#include "CLHEP/Random/Random.h"
#include "CLHEP/Random/Ranlux64Engine.h"
#include <string.h>
#include <cmath>	// for ldexp() and abs()
#include <stdlib.h>	// for abs(int)

using namespace std;

namespace CLHEP {

static const int MarkerLen = 64; // Enough room to hold a begin or end marker. 


// Number of instances with automatic seed selection
int Ranlux64Engine::numEngines = 0;

// Maximum index into the seed table
int Ranlux64Engine::maxIndex = 215;

double Ranlux64Engine::twoToMinus_32;
double Ranlux64Engine::twoToMinus_48;
double Ranlux64Engine::twoToMinus_49;

Ranlux64Engine::Ranlux64Engine()
{
   luxury = 1;
   int cycle    = abs(int(numEngines/maxIndex));
   int curIndex = abs(int(numEngines%maxIndex));
   numEngines +=1;
   long mask = ((cycle & 0x007fffff) << 8);
   long seedlist[2];
   HepRandom::getTheTableSeeds( seedlist, curIndex );
   seedlist[0] ^= mask;
   seedlist[1] = 0;

   setSeeds(seedlist, luxury);
   advance ( 8 );  		// Discard some iterations and ensure that
				// this sequence won't match one where seeds 
				// were provided.
}

Ranlux64Engine::Ranlux64Engine(long seed, int lux)
{
   luxury = lux;
   long seedlist[2]={seed,0};
   setSeeds(seedlist, lux);
   advance ( 2*lux + 1 );  	// Discard some iterations to use a different 
				// point in the sequence.  
}

Ranlux64Engine::Ranlux64Engine(int rowIndex, int colIndex, int lux)
{
   luxury = lux;
   int cycle = abs(int(rowIndex/maxIndex));
   int   row = abs(int(rowIndex%maxIndex));
   // int   col = abs(int(colIndex%2)); // But this is never used!
   long mask = (( cycle & 0x000007ff ) << 20 );
   long seedlist[2]; 
   HepRandom::getTheTableSeeds( seedlist, row );
   seedlist[0] ^= mask;
   seedlist[1]= 0;
   setSeeds(seedlist, lux);
}

Ranlux64Engine::Ranlux64Engine( std::istream& is )
{
  is >> *this;
}

Ranlux64Engine::~Ranlux64Engine() {}

Ranlux64Engine::Ranlux64Engine(const Ranlux64Engine &p)
{
  *this = p;
}

Ranlux64Engine & Ranlux64Engine::operator=( const Ranlux64Engine &p ) {
  if (this != &p ) {
    theSeed  = p.theSeed;
    theSeeds = p.theSeeds;
    for (int i=0; i<12; ++i) {
      randoms[i] = p.randoms[i];
    }
    pDiscard = p.pDiscard; 
    pDozens  = p.pDozens;
    endIters = p.endIters;
    luxury   = p.luxury;
    index    = p.index;
    carry    = p.carry;
  }
  return *this;
}


double Ranlux64Engine::flat() {
  // Luscher improves the speed by computing several numbers in a shot,
  // in a manner similar to that of the Tausworth in DualRand or the Hurd
  // engines.  Thus, the real work is done in update().  Here we merely ensure
  // that zero, which the algorithm can produce, is never returned by flat().

  if (index <= 0) update();
  return randoms[--index] + twoToMinus_49;
}

void Ranlux64Engine::update() {
  // Update the stash of twelve random numbers.  
  // When this routione is entered, index is always 0.  The randoms 
  // contains the last 12 numbers in the sequents:  s[0] is x[a+11], 
  // s[1] is x[a+10] ... and s[11] is x[a] for some a.  Carry contains
  // the last carry value (c[a+11]).
  //
  // The recursion relation (3) in Luscher's note says 
  //   delta[n] = x[n-s] = x[n-r] -c[n-1] or for n=a+12,
  //   delta[a+12] = x[a+7] - x[a] -c[a+11] where we use r=12, s=5 per eqn. (7)
  // This reduces to 
  // s[11] = s[4] - s[11] - carry.
  // The next number similarly will be given by s[10] = s[3] - s[10] - carry,
  // and so forth until s[0] is filled.
  // 
  // However, we need to skip 397, 202 or 109 numbers - these are not divisible 
  // by 12 - to "fare well in the spectral test".  

  advance(pDozens);

  // Since we wish at the end to have the 12 last numbers in the order of 
  // s[11] first, till s[0] last, we will have to do 1, 10, or 1 iterations 
  // and then re-arrange to place to get the oldest one in s[11].
  // Generically, this will imply re-arranging the s array at the end,
  // but we can treat the special case of endIters = 1 separately for superior
  // efficiency in the cases of levels 0 and 2.

  register double  y1;

  if ( endIters == 1 ) {  	// Luxury levels 0 and 2 will go here
    y1 = randoms[ 4] - randoms[11] - carry;
    if ( y1 < 0.0 ) {
      y1 += 1.0;			
      carry = twoToMinus_48;
    } else {
      carry = 0.0;
    }
    randoms[11] = randoms[10];  
    randoms[10] = randoms[ 9];  
    randoms[ 9] = randoms[ 8];  
    randoms[ 8] = randoms[ 7];  
    randoms[ 7] = randoms[ 6];  
    randoms[ 6] = randoms[ 5];  
    randoms[ 5] = randoms[ 4];  
    randoms[ 4] = randoms[ 3];  
    randoms[ 3] = randoms[ 2];  
    randoms[ 2] = randoms[ 1];  
    randoms[ 1] = randoms[ 0];  
    randoms[ 0] = y1;

  } else {

    int m, nr, ns;
    for ( m = 0, nr = 11, ns = 4; m < endIters; ++m, --nr ) {
      y1 = randoms [ns] - randoms[nr] - carry;
      if ( y1 < 0.0 ) {
        y1 += 1.0;
        carry = twoToMinus_48;
      } else {
        carry = 0.0;
      }
      randoms[nr] = y1;
      --ns;
      if ( ns < 0 ) {
        ns = 11;
      }
    } // loop on m

    double temp[12];
    for (m=0; m<12; m++) {
      temp[m]=randoms[m];
    }

    ns = 11 - endIters;
    for (m=11; m>=0; --m) {
      randoms[m] = temp[ns];
      --ns;
      if ( ns < 0 ) {
        ns = 11;
      }
    } 

  }

  // Now when we return, there are 12 fresh usable numbers in s[11] ... s[0]

  index = 11;

} // update()

void Ranlux64Engine::advance(int dozens) {

  register double  y1, y2, y3;
  register double  cValue = twoToMinus_48;
  register double  zero = 0.0;
  register double  one  = 1.0;

		// Technical note:  We use Luscher's trick to only do the
		// carry subtraction when we really have to.  Like him, we use 
		// three registers instead of two so that we avoid sequences
		// like storing y1 then immediately replacing its value:
		// some architectures lose time when this is done.

  		// Luscher's ranlxd.c fills the stash going
		// upward.  We fill it downward to save a bit of time in the
		// flat() routine at no cost later.  This means that while
		// Luscher's ir is jr+5, our n-r is (n-s)-5.  (Note that
		// though ranlxd.c initializes ir and jr to 11 and 7, ir as
		// used is 5 more than jr because update is entered after 
		// incrementing ir.)  
		//

		// I have CAREFULLY checked that the algorithms do match
		// in all details.

  int k;
  for ( k = dozens; k > 0; --k ) {

    y1 = randoms[ 4] - randoms[11] - carry;

    y2 = randoms[ 3] - randoms[10];
    if ( y1 < zero ) {
      y1 += one;			
      y2 -= cValue;
    }
    randoms[11] = y1;

    y3 = randoms[ 2] - randoms[ 9];
    if ( y2 < zero ) {
      y2 += one;			
      y3 -= cValue;
    }
    randoms[10] = y2;

    y1 = randoms[ 1] - randoms[ 8];
    if ( y3 < zero ) {
      y3 += one;			
      y1 -= cValue;
    }
    randoms[ 9] = y3;

    y2 = randoms[ 0] - randoms[ 7];
    if ( y1 < zero ) {
      y1 += one;			
      y2 -= cValue;
    }
    randoms[ 8] = y1;

    y3 = randoms[11] - randoms[ 6];
    if ( y2 < zero ) {
      y2 += one;			
      y3 -= cValue;
    }
    randoms[ 7] = y2;

    y1 = randoms[10] - randoms[ 5];
    if ( y3 < zero ) {
      y3 += one;			
      y1 -= cValue;
    }
    randoms[ 6] = y3;

    y2 = randoms[ 9] - randoms[ 4];
    if ( y1 < zero ) {
      y1 += one;			
      y2 -= cValue;
    }
    randoms[ 5] = y1;

    y3 = randoms[ 8] - randoms[ 3];
    if ( y2 < zero ) {
      y2 += one;			
      y3 -= cValue;
    }
    randoms[ 4] = y2;

    y1 = randoms[ 7] - randoms[ 2];
    if ( y3 < zero ) {
      y3 += one;			
      y1 -= cValue;
    }
    randoms[ 3] = y3;

    y2 = randoms[ 6] - randoms[ 1];
    if ( y1 < zero ) {
      y1 += one;			
      y2 -= cValue;
    }
    randoms[ 2] = y1;

    y3 = randoms[ 5] - randoms[ 0];
    if ( y2 < zero ) {
      y2 += one;			
      y3 -= cValue;
    }
    randoms[ 1] = y2;

    if ( y3 < zero ) {
      y3 += one;			
      carry = cValue;
    }
    randoms[ 0] = y3; 

  } // End of major k loop doing 12 numbers at each cycle

} // advance(dozens)

void Ranlux64Engine::flatArray(const int size, double* vect) {
  for( int i=0; i < size; ++i ) {
    vect[i] = flat(); 
  }
}

void Ranlux64Engine::setSeed(long seed, int lux) {

// The initialization is carried out using a Multiplicative
// Congruential generator using formula constants of L'Ecuyer
// as described in "A review of pseudorandom number generators"
// (Fred James) published in Computer Physics Communications 60 (1990)
// pages 329-344

  twoToMinus_32 = ldexp (1.0, -32);
  twoToMinus_48 = ldexp (1.0, -48);
  twoToMinus_49 = ldexp (1.0, -49);

  const int ecuyer_a(53668);
  const int ecuyer_b(40014);
  const int ecuyer_c(12211);
  const int ecuyer_d(2147483563);

  const int lux_levels[3] = {109, 202, 397};
  theSeed = seed;

  if( (lux > 2)||(lux < 0) ){
     pDiscard = (lux >= 12) ? (lux-12) : lux_levels[1];
  }else{
     pDiscard = lux_levels[luxury];
  }
  pDozens  = pDiscard / 12;
  endIters = pDiscard % 12;

  long init_table[24];
  long next_seed = seed;
  long k_multiple;
  int i;
  
  for(i = 0;i != 24;i++){
     k_multiple = next_seed / ecuyer_a;
     next_seed = ecuyer_b * (next_seed - k_multiple * ecuyer_a)
                                       - k_multiple * ecuyer_c;
     if(next_seed < 0) {
	next_seed += ecuyer_d;
     }
     init_table[i] = next_seed & 0xffffffff;
  }    

  for(i = 0;i < 12; i++){
     randoms[i] = (init_table[2*i  ]      ) * 2.0 * twoToMinus_32 +
                  (init_table[2*i+1] >> 15) * twoToMinus_48;
  }

  carry = 0.0;
  if ( randoms[11] == 0. ) carry = twoToMinus_48;
  index = 11;

} // setSeed()

void Ranlux64Engine::setSeeds(const long * seeds, int lux) {
  setSeed( *seeds ? *seeds : 32767, lux );
  theSeeds = seeds;
}

void Ranlux64Engine::saveStatus( const char filename[] ) const
{
   std::ofstream outFile( filename, std::ios::out ) ;
   if (!outFile.bad()) {
     outFile << theSeed << std::endl;
     for (int i=0; i<12; ++i) {
       outFile <<std::setprecision(20) << randoms[i]    << std::endl;
     }
     outFile << std::setprecision(20) << carry << " " << index << std::endl;
     outFile << luxury << " " << pDiscard << std::endl;
   }
}

void Ranlux64Engine::restoreStatus( const char filename[] )
{
   std::ifstream inFile( filename, std::ios::in);

   if (!inFile.bad() && !inFile.eof()) {
     inFile >> theSeed;
     for (int i=0; i<12; ++i) {
       inFile >> randoms[i];
     }
     inFile >> carry; inFile >> index;
     inFile >> luxury; inFile >> pDiscard;
     pDozens  = pDiscard / 12;
     endIters = pDiscard % 12;
   }
}

void Ranlux64Engine::showStatus() const
{
   std::cout << std::endl;
   std::cout << "--------- Ranlux engine status ---------" << std::endl;
   std::cout << " Initial seed = " << theSeed << std::endl;
   std::cout << " randoms[] = ";
   for (int i=0; i<12; ++i) {
     std::cout << randoms[i] << std::endl;
   }
   std::cout << std::endl;
   std::cout << " carry = " << carry << ", index = " << index << std::endl;
   std::cout << " luxury = " << luxury << " pDiscard = " 
						<< pDiscard << std::endl;
   std::cout << "----------------------------------------" << std::endl;
}

std::ostream & operator << ( std::ostream& os, const Ranlux64Engine& e )
{
   char beginMarker[] = "Ranlux64Engine-begin";
   char endMarker[]   = "Ranlux64Engine-end";

   os << " " << beginMarker << " ";
   os << e.theSeed << " ";
   for (int i=0; i<12; ++i) {
     os << std::setprecision(20) << e.randoms[i] << std::endl;
   }
   os << std::setprecision(20) << e.carry << " " << e.index << " ";
   os << e.luxury << " " << e.pDiscard << " ";
   os << endMarker << " ";
   return os;
}

std::istream & operator >> ( std::istream& is, Ranlux64Engine& e )
{
  char beginMarker [MarkerLen];
  char endMarker   [MarkerLen];

  is >> std::ws;
  is.width(MarkerLen);  // causes the next read to the char* to be <=
			// that many bytes, INCLUDING A TERMINATION \0 
			// (Stroustrup, section 21.3.2)
  is >> beginMarker;
  if (strcmp(beginMarker,"Ranlux64Engine-begin")) {
     is.clear(std::ios::badbit | is.rdstate());
     std::cerr << "\nInput stream mispositioned or"
	       << "\nRanlux64Engine state description missing or"
	       << "\nwrong engine type found." << std::endl;
     return is;
  }
  is >> e.theSeed;
  for (int i=0; i<12; ++i) {
     is >> e.randoms[i];
  }
  is >> e.carry; is >> e.index;
  is >> e.luxury; is >> e.pDiscard;
  e.pDozens  = e.pDiscard / 12;
  e.endIters = e.pDiscard % 12;
  is >> std::ws;
  is.width(MarkerLen);  
  is >> endMarker;
  if (strcmp(endMarker,"Ranlux64Engine-end")) {
     is.clear(std::ios::badbit | is.rdstate());
     std::cerr << "\nRanlux64Engine state description incomplete."
	       << "\nInput stream is probably mispositioned now." << std::endl;
     return is;
  }
  return is;
}

}  // namespace CLHEP
