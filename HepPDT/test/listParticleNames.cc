// $Id: listParticleNames.cc,v 1.1.4.1 2004/04/21 23:06:34 garren Exp $
// ----------------------------------------------------------------------
// listParticleNames.cc
//
// list all known HepPDT particle names
//
// Usage:  listParticleNames 
//
// ----------------------------------------------------------------------

#include <fstream>
#include <iostream>

#include "CLHEP/HepPDT/ParticleName.hh"

int main()
{
    const char outfile[] = "listParticleNames.out";
    // open the output file
    std::ofstream wpdfile( outfile );
    if( !wpdfile ) { 
      std::cerr << "cannot open " << outfile << std::endl;
      exit(-1);
    }
    // write the particle names
    HepPDT::listHepPDTParticleNames( wpdfile );
}
