// $Id: RanecuEngine.h,v 1.2 2003/07/17 19:20:02 garren Exp $
// -*- C++ -*-
//
// -----------------------------------------------------------------------
//                             HEP Random
//                         --- RanecuEngine ---
//                          class header file
// -----------------------------------------------------------------------
// This file is part of Geant4 (simulation toolkit for HEP).
//
// RANECU Random Engine - algorithm originally written in FORTRAN77
//                        as part of the MATHLIB HEP library.
// The initialisation is carried out using a Multiplicative Congruential
// generator using formula constants of L'Ecuyer as described in "F.James,
// Comp. Phys. Comm. 60 (1990) 329-344".
// Seeds are taken from a seed table given an index, the getSeed() method
// returns the current index in the seed table, the getSeeds() method
// returns a pointer to the couple of seeds stored in the local table of
// seeds at the current index.

// =======================================================================
// Gabriele Cosmo - Created: 2nd February 1996
//                - Minor corrections: 31st October 1996
//                - Added methods for engine status: 19th November 1996
//                - setSeed() now has default dummy argument
//                  set to zero: 11th July 1997
//                - Added default index to setSeeds(): 16th Oct 1997
// J.Marraffino   - Added stream operators and related constructor.
//                  Added automatic seed selection from seed table and
//                  engine counter: 16th Feb 1998
// Ken Smith      - Added conversion operators:  6th Aug 1998
// =======================================================================

#ifndef RanecuEngine_h
#define RanecuEngine_h 1

#include "CLHEP/Random/RandomEngine.h"

namespace CLHEP {

/**
 * @author <Gabriele.Cosmo@cern.ch>
 * @ingroup random
 */
class RanecuEngine : public HepRandomEngine {

public:

  RanecuEngine(std::istream& is);
  RanecuEngine();
  RanecuEngine(int index);
  virtual ~RanecuEngine();
  // Constructors and destructor.

  RanecuEngine(const RanecuEngine &p);
  // Copy constructor

  RanecuEngine & operator = (const RanecuEngine &p);
  // Overloaded assignment operator, to retrieve the engine status.

  double flat();
  // Returns a pseudo random number between 0 and 1 
  // (excluding the end points)

  void flatArray (const int size, double* vect);
  // Fills an array "vect" of specified size with flat random values.

  void setIndex (long index);
  // Sets the state of the algorithm according to "index", the position
  // in the local table of seeds.
 
  void setSeed (long index, int dum=0);
  // Resets the state of the algorithm according to "index", the position
  // in the static table of seeds stored in HepRandom.

  void setSeeds (const long* seeds, int index=-1);
  // Sets the state of the algorithm according to the array of seeds
  // "seeds" containing two seed values to be stored in the local table at
  // "index" position.

  void saveStatus( const char filename[] = "Ranecu.conf" ) const;
  // Saves on file Ranecu.conf the current engine status.

  void restoreStatus( const char filename[] = "Ranecu.conf" );
  // Reads from file Ranecu.conf the last saved engine status
  // and restores it.

  void showStatus() const;
  // Dumps the engine status on the screen.

  operator unsigned int();
  // 32-bit int flat, faster in this case

  friend std::ostream& operator<< (std::ostream& os, const RanecuEngine& e);
  friend std::istream& operator>> (std::istream& is,       RanecuEngine& e);

protected:

  // Suggested L'ecuyer coefficients for portable 32 bits generators.
  
  const int ecuyer_a, ecuyer_b, ecuyer_c, ecuyer_d, ecuyer_e, ecuyer_f;
  const int shift1, shift2;
  const double prec;

private:

  // Members defining the current state of the generator.

  const int maxSeq;
  long table[215][2];
  int seq;
  static int numEngines;

};

}  // namespace CLHEP

#endif