#ifndef CMR_PRICER_H
#define CMR_PRICER_H

#include "lp_interface.hpp"
#include "datagroups.hpp"
#include "hypergraph.hpp"
#include "price_util.hpp"

extern "C" {
#include <concorde/INCLUDE/tsp.h>
}

#include <queue>
#include <unordered_map>
#include <vector>

namespace CMR {

/** Namespace for manners related to pricing sets of edges. */
namespace Price {

/** Class for pricing edges not in the CoreLP. */
class Pricer {
public:
    /** Construct a Pricer.
     * @param[in] _relax the Relaxation for grabbing dual values
     * @param[in] _inst the TSP instance for generating edges
     * @param[in] _ext_cuts the HyperGraph representation of the cuts in 
     * \p _relax.
     */
    Pricer(const LP::Relaxation &_relax, const Data::Instance &_inst,
           const Sep::ExternalCuts &_ext_cuts);

    Pricer(const Pricer &P) = delete;
    Pricer &operator=(const Pricer &P) = delete;

    ~Pricer();

    /** Generate edges to add to the CoreLP.
     * @param[in] piv_stat the PivType from the solution when edge generation 
     * is called. The partial edge set is scanned iff \p piv_stat is 
     * PivType::Tour. Full edge set may be scanned if \p piv_stat is 
     * PivType::FathomedTour, or PivType::Tour with a small enough graph, or
     * if a partial scan finds no edges to add. 
     * @returns a ScanStat indicating the outcome of the pricing scan.
     */
    ScanStat add_edges(LP::PivType piv_stat);

private:
    /// Scan the 50-nearest edges.
    /// @returns true iff negative reduced cost edges were found.
    ScanStat partial_scan();

    /// Scan the full edge set.
    /// @returns true iff negative reduced cost edges were found.
    ScanStat full_scan(); 
    
    const LP::Relaxation &relax;
    const Data::Instance &inst;
    const Sep::ExternalCuts &ext_cuts;

    const int gen_max;

    std::vector<int> gen_elist;
    std::vector<double> gen_elen;

    std::vector<double> node_pi; //!< pi values for degree eqns.
    std::vector<double> node_pi_est; //!< estimated node pi for dominos.
    
    std::vector<double> cut_pi; //!< pi values for cuts.

    std::unordered_map<Sep::Clique, double> clique_pi;

    CCtsp_edgegenerator eg_inside;

    std::vector<edge> edge_q; //!< Queue of edges for inclusion.

};

}
}

#endif
