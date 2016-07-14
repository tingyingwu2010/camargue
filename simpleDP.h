#ifndef PSEP_SIMPLEDP_H
#define PSEP_SIMPLEDP_H

extern "C" {
#include "../programs/concorde/concorde.h"
}

#include "tooth.h"

class PSEP_SimpleDP {
 public:
  PSEP_SimpleDP(std::vector<int> & _tour_nodes, SupportGraph & _G,
		std::vector<int> & _edge_marks, std::vector<int> & _elist,
		std::vector<double>& _ecap) :
  G_s(_G), support_elist(_elist), support_ecap(_ecap),
    candidates(_tour_nodes, _G, _edge_marks) {}
		
  int separate(const int max_cutcount);
  void build_light_cuttree();
  void test_build_collection();

 private:
  SupportGraph &G_s;

  std::vector<int> &support_elist;
  std::vector<double> &support_ecap;

  std::vector<std::shared_ptr<PSEP_CandTooth::SimpleTooth> > light_nodes;
  std::vector<int> cut_elist;
  std::vector<double> cut_ecap;
  std::vector<int> cut_marks;
  std::vector<bool> node_marks;
  
  PSEP_CandTooth candidates;
};

#endif
