/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 *
 *                MAIN TSP SOLVER CLASS DEFINITION
 *
 * This is the overarching class responsible for managing problem data and 
 * executing different solution protocols. 
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef TSP_SOLVER_H
#define TSP_SOLVER_H

#include <vector>
#include <memory>

#include "datagroups.h"
#include "purecut.h"
#include "ABC.h"
#include "pivplan.h"
#include "PSEP_util.h"


namespace PSEP{
class TSPSolver {
 public:
  /*  The default constructor for the TSPSolver class.
   * ONE OF fname or randprob may be used to initialize the problem
   * fname.empty() implies use of randprob, randprob.nodecount of zero
   * implies use of fname. The command line argument parser is responsible
   * for giving precisely one valid argument. (PSEP.cpp in this case).
   * If both are somehow nonempty, the filename will be chosen
   *
   * fname: a TSP instance with .tsp suffix using TSPLIB format
   * ranndprob: parameters for generating a random problem; see
   *    PSEP_util.h for information
   * prefs: LP solution preferences, see PSEP_util.h for info
   * dat: a unique pointer to an uninitialized CCdatagroup object
   */
  TSPSolver(const std::string &fname, PSEP::RandProb &randprob,
	    PSEP::LP::Prefs _prefs,
	    std::unique_ptr<CCdatagroup> &dat);

  /*
   * The function to invoke the TSP solver with solution protocol specified
   * by  solmeth. See PSEP_util.h and below for info
   */
  int call(PSEP::SolutionProtocol solmeth);
  
 private:
  /*
   * These are the data categories used by various aspects of the solver,
   * see datagroups.h for more info.
   */
  Data::GraphGroup GraphGroup;
  Data::BestGroup BestGroup;
  Data::SupportGroup SupportGroup;
  Data::LPGroup LPGroup;

  /* These are pointers to solution protocol classes
   * See purecut.h and ABC.h for info
   */
  std::unique_ptr<PSEP::PureCut> PureCut;
  std::unique_ptr<PSEP::ABC> ABC;
};
}

#endif


