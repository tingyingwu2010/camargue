#ifndef CMR_SOLVER_H
#define CMR_SOLVER_H

#include "datagroups.hpp"
#include "core_lp.hpp"
#include "cutcontrol.hpp"
#include "util.hpp"

#include <string>

namespace CMR {

class Solver {
public:
    /** Construct a Solver from a TSPLIB instance. */
    Solver(const std::string &tsp_fname, const int seed,
           const CMR::OutPrefs outprefs);

    /** Construct Solver from TSPLIB instance with starting tour from file. */
    Solver(const std::string &tsp_fname, const std::string &tour_fname,
           const int seed, const CMR::OutPrefs outprefs);

    /** Construct Solver from randomly generated Euclidean TSP instance. */
    Solver(const int seed, const int node_count, const int gridsize,
           const CMR::OutPrefs outprefs);

    CMR::LP::PivType cutting_loop();

    

private:
    void report_piv(CMR::LP::PivType piv, int round);
    
    CMR::Data::Instance tsp_instance;
    CMR::Data::KarpPartition karp_part;
    CMR::Data::GraphGroup graph_data;
    CMR::Data::BestGroup best_data;

    CMR::LP::CoreLP core_lp;

    CMR::OutPrefs output_prefs;
};

}

#endif
