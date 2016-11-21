/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/** @file
 * @brief EXACT PRIMAL LIGHT SIMPLE DP SEPARATION
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "DPgraph.hpp"
#include "tooth.hpp"
#include "datagroups.hpp"
#include "cuts.hpp"

namespace PSEP {

template<> class Cut<dominoparity> {
public:
  Cut<dominoparity>(PSEP::Data::GraphGroup &graph_dat,
		    PSEP::Data::BestGroup &best_dat,
		    PSEP::Data::SupportGroup &supp_dat,
		    PSEP::CutQueue<dominoparity> &_dp_q) :
    candidates(graph_dat.delta, graph_dat.edge_marks,
	       best_dat.best_tour_nodes, best_dat.perm,
	       supp_dat.G_s, supp_dat.support_elist, supp_dat.support_ecap),
    dp_q(_dp_q) {}

  int cutcall();

protected:
  int separate();
  int add_cuts();

private:
  PSEP::CandidateTeeth candidates;
  PSEP::CutQueue<PSEP::dominoparity> &dp_q;
  //PSEP::CutQueue<PSEP::externalDP> &external_q;
};

}