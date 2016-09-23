#ifndef PSEP_DOMINOS_H
#define PSEP_DOMINOS_H

#include<vector>

#include "lp.hpp"
#include "cuts.hpp"
#include "Graph.hpp"
#include "simpleDP.hpp"

namespace PSEP{
  template<> class Cut<domino> {
  public:
    Cut<domino>(std::vector<int> &_edge_marks,
		IntPairMap &_edge_lookup,
		std::vector<int> &_tour_nodes, std::vector<int> &_perm,
		PSEPlp &_m_lp, std::vector<double> &_m_lp_edges,
		SupportGraph &_G_s, std::vector<int> &_support_elist,
		std::vector<double> &_support_ecap):
    m_lp(_m_lp), m_lp_edges(_m_lp_edges),
      SimpleDP(_tour_nodes, _perm, _G_s, _edge_marks, _support_elist,
	       _support_ecap, _edge_lookup, _m_lp) {}
    int cutcall();

  protected:
    int separate();
    int add_cut();

  private:
    int parse_coeffs();

    PSEPlp &m_lp;
    std::vector<double> &m_lp_edges;

    PSEP::SimpleDP SimpleDP;

    double rhs;
    std::vector<double> agg_coeffs;

    std::unique_ptr<domino> best;
  };
}

#endif
