#ifndef PSEP_LPWORK_H
#define PSEP_LPWORK_H

#include<iostream>

#include "lp.hpp"
#include "PSEP_util.hpp"
#include "Graph.hpp"
#include "datagroups.hpp"
#include "LPprune.hpp"

namespace PSEP {
  class PureCut;
  class CutControl;
}

namespace PSEP {
  namespace LP {
    class Core {
    public:
    Core(Data::LPGroup &LPGroup, Data::GraphGroup &GraphGroup,
	 Data::SupportGroup &SupportGroup, Data::BestGroup &BestGroup,
	 PSEP::LP::CutPrune &_LPPrune, PSEP::OutPrefs &_outprefs) :
      LPPrune(_LPPrune),
      m_lp(LPGroup.m_lp), m_graph(GraphGroup.m_graph), prefs(LPGroup.prefs),
      outprefs(_outprefs),
      old_colstat(LPGroup.old_colstat), old_rowstat(LPGroup.old_rowstat),
      frac_colstat(LPGroup.frac_colstat), frac_rowstat(LPGroup.frac_rowstat),
      m_lp_edges(LPGroup.m_lp_edges), G_s(SupportGroup.G_s),
      support_indices(SupportGroup.support_indices),
      support_elist(SupportGroup.support_elist),
      support_ecap(SupportGroup.support_ecap),
      best_tour_edges(BestGroup.best_tour_edges),
      best_tour_nodes(BestGroup.best_tour_nodes), perm(BestGroup.perm),
      m_min_tour_value(BestGroup.min_tour_value),
      island(GraphGroup.island), delta(GraphGroup.delta),
      edge_marks(GraphGroup.edge_marks){
      basis_init(); //TODO: maybe make a basis structure to better enforce
      //encapsulation
      change_pricing();
    }
    
      bool is_dual_feas();
      bool is_integral();

      int is_best_tour_feas(bool &result);

      int factor_basis();
      int single_pivot();
      int nondegenerate_pivot();
      int pivot_back();
      int primal_opt();

      int add_connect_cuts(PivType &piv_stat);
      int del_connect_cut();

      int rebuild_basis(bool prune);
      int rebuild_basis(int &numremoved, IntPair skiprange,
			std::vector<int> &delset);
      int basis_init();
  
      int pivot_until_change(PSEP::LP::PivType &pivot_status);


      double get_obj_val();
      double set_edges();
      int set_support_graph();

      int update_best_tour();
      bool test_new_tour();

      int numrows(){ return PSEPlp_numrows(&m_lp); }
      int numcols(){ return PSEPlp_numcols(&m_lp); }

      void change_pricing();

    private:
      int dual_pivot();
      
      PSEP::LP::CutPrune &LPPrune;
      PSEPlp &m_lp;
      Graph &m_graph;

      PSEP::LP::Prefs prefs;
      PSEP::OutPrefs &outprefs;

      friend class TSP_Solver;
      friend class PSEP::PureCut;
      friend class PSEP::CutControl;
      std::vector<int> &old_colstat;
      std::vector<int> &old_rowstat;
      std::vector<int> &frac_colstat;
      std::vector<int> &frac_rowstat;

      std::vector<double> &m_lp_edges;

      SupportGraph &G_s;
  
      std::vector<int> &support_indices;
      std::vector<int> &support_elist;
      std::vector<double> &support_ecap;


      std::vector<int> &best_tour_edges;
      std::vector<int> &best_tour_nodes;
      std::vector<int> &perm;

      std::vector<double> best_tour_edges_lp;

      double &m_min_tour_value;

      int icount;
      IntPair connect_cut_range;
      std::vector<int> &island;
      std::vector<int> &delta;
      std::vector<int> &edge_marks;
    };
  }
}

#endif