//////////////////////////////////////////////////////////////////////////
// Matt.Dobbs@Cern.CH, December 1999
// November 2000, updated to use Pythia 6.1
// example of generating events with Pythia 
// using HepMC/PythiaWrapper.h 
// Events are read into the HepMC event record from the FORTRAN HEPEVT 
// common block using the IO_HEPEVT strategy -- nothing is done with them.
// This program is just used to find the total time required to transfer
// from HEPEVT into the HepMC event record.
//////////////////////////////////////////////////////////////////////////
// To Compile: go to the HepMC directory and type:
// gmake examples/example_MyPythiaOnlyTo HepMC.exe
//
// See comments in examples/example_MyPythia.cxx regarding the HEPEVT wrapper.
//

#include "CLHEP/HepMC/defs.h"
#include <iostream>
#include "CLHEP/HepMC/include/PythiaWrapper.h"
#include "CLHEP/HepMC/ConvertHEPEVT.h"
#include "CLHEP/HepMC/GenEvent.h"
    
int main() { 	
    //
    //........................................HEPEVT
    // Pythia 6.1 uses HEPEVT with 4000 entries and 8-byte floating point
    //  numbers. We need to explicitly pass this information to the 
    //  HEPEVT_Wrapper.
    //
    //HepMC::HEPEVT_Wrapper::set_max_number_entries(4000);
    //HepMC::HEPEVT_Wrapper::set_sizeof_real(8);
    //	
    //........................................PYTHIA INITIALIZATIONS
    // (Some platforms may require the initialization of pythia PYDATA block 
    //  data as external - if you get pythia initialization errors try 
    //  commenting in/out the below call to initpydata() )
    // initpydata();
    //
    // Select W+gamma process (process number 20) 
    // (here we have to be careful of C/F77 differences: arrays in C 
    //  start at 0, F77 at 1, so we need to subtract 1 from the process #)
    pysubs.msel=0;
    pysubs.msub[20-1] = 1;
    // set random number seed (mandatory!)
    pydatr.mrpy[0]=55122 ;
    // Tell Pythia not to write multiple copies of particles in event record.
    pypars.mstp[128-1] = 2;
    // Example of setting a Pythia parameter: set the top mass 
    pydat2.pmas[1-1][6-1]= 175;  
    //
    // Call pythia initialization
    call_pyinit( "CMS", "p", "p", 14000. );
    //
    //........................................HepMC INITIALIZATIONS
    //
    // Instantiate the HEPEVT conversion class.
    HepMC::ConvertHEPEVT conv;
    //
    //........................................EVENT LOOP
    for ( int i = 1; i <= 100; i++ ) {
	if ( i%50==1 ) std::cout << "Processing Event Number " 
				 << i << std::endl;
	call_pyevnt();      // generate one event with Pythia
	// pythia pyhepc routine convert common PYJETS in common HEPEVT
	call_pyhepc( 1 );
	HepMC::GenEvent* evt = conv.getGenEventfromHEPEVT();
	//
	//.......................USER WOULD PROCESS EVENT HERE
	// Begin Peter sherwood's suggested code:

        //count particles 
	int npart =0;
        HepMC::GenEvent::particle_const_iterator it= evt->particles_begin();
        HepMC::GenEvent::particle_const_iterator en= evt->particles_end();
        for(;it!=en;++it) ++npart;

        //check event size
        int ndumpv=evt->vertices_size();
        int ndumpp=evt->particles_size();

	if ( npart!=ndumpp ) {
	    std::cout << "Event " << i << std::endl;
	    std::cout << "Total vertices count as dumped "<<ndumpv<< std::endl;
	    std::cout << "Total particle count as copied "<<npart<< std::endl;
	    std::cout << "Total particle count as dumped "<<ndumpp<< std::endl;
	}

	//
	// we also need to delete the created event from memory
	delete evt;
    }
    //........................................TERMINATION
    // write out some information from Pythia to the screen
    call_pystat( 1 );    

    return 0;
}


 
