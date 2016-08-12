#ifndef PSEP_GENCUTS_H
#define PSEP_GENCUTS_H

#include<array>
#include<vector>

#include "lp.h"
#include "cuts.h"

namespace PSEP{
  template<> class Cut<general> {
  public:
    Cut<general>(const bool _gomory, const bool _disj, const bool _mir,
		 std::vector<int> &_best_tour_edges, PSEPlp &_m_lp,
		 std::vector<double> &_m_lp_edges,
		 std::vector<int> &_support_indices) :
    gencuts(_gomory, _disj, _mir), best_tour_edges(_best_tour_edges),
      m_lp(_m_lp), m_lp_edges(_m_lp_edges), support_indices(_support_indices){}

    int separate(const double piv_val);
    int separate(const int edge, const double piv_val);

  private:
    int init_mip(const double piv_val);
    int revert_lp();

    int make_all_binary();
    int make_binary(const int edge);

    int check_cuts();
    int num_added(int &frac, int &disj, int &mir);

    int deletion_row;
    static constexpr int max_cuts = 10;
    
    PSEP::general gencuts;
    std::vector<int> &best_tour_edges;
    PSEPlp &m_lp;
    std::vector<double> &m_lp_edges;
    std::vector<int> &support_indices;

    struct mip_cut_candidates {
    mip_cut_candidates(const int numcols) :
      coefficient_vectors(max_cuts, std::vector<double>(numcols)),
      index_vectors(max_cuts, std::vector<int>(numcols)) {}
           
      std::vector<std::vector<double>> coefficient_vectors;
      std::vector<std::vector<int>> index_vectors;
      std::array<char, max_cuts> senses;
      std::array<double, max_cuts> rhs_array;
      int next_cut;
    };

    int branchcallback (CPXCENVptr xenv, void *cbdata, int wherefrom,
			   void *cbhandle, int brtype, int brset, int nodecnt,
			   int bdcnt, const double *nodeest, const int *nodebeg,
			   const int *xindex, const char *lu, const int *bd,
			       int *useraction_p);
  };
}

#endif
