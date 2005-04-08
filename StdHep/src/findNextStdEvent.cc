// ----------------------------------------------------------------------
//
// findNextStdEvent.cc
// Author: Lynn Garren
//
// read from an ascii file
// process a single event - do not combine events
// ----------------------------------------------------------------------

#include <iostream>
#include <fstream>
#include <string>

#include "CLHEP/StdHep/defs.h"
#include "CLHEP/StdHep/ReadStdHep.hh"
#include "CLHEP/HepMC/ReadHepMC.h"
#include "CLHEP/StdHep/StdEvent.hh"

namespace StdHep  {

StdEvent * findNextStdEvent( std::istream & is )
{
    // check the state of the input stream
    if ( !is ) return NULL;
    //
    // find an event block
    std::string type("StdEvent");
    bool ok = findBlockType( is, type );
    // read the event
    if( ok ) {
	StdEvent * evt = readStdEvent( is );
        return evt;
    }
    // no event has been found
    return NULL;
}

}	// StdHep
