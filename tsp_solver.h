#ifndef TSP_SOLVER_H
#define TSP_SOLVER_H

#include <vector>
#include <list>
#include <iostream>
#include <math.h>
#include <memory>

#include <cplex.h>

extern "C" {
#include "../programs/concorde/concorde.h"
}

#include "Graph.h"
#include "profiler.h"
#include "segcuts.h"
#include "blossom.h"
#include "lp.h"
#include "PSEP_util.h"



class TSP_Solver {
 public:
  //CONSTRUCTOR & DESTRUCTOR -- tsp_constructors.cpp
  TSP_Solver(Graph &graph, const std::vector<int> &lk_node_indices);
  ~TSP_Solver();

  //MAIN OPERATIONS -- tsp_solver.cpp
  int pivot_until_change(int *pivot_status_p);
  int pivot_back();
  bool update_current_tour_indices(std::vector<int> &tour_indices);
  int update_best_tour();

  //CUTTING PLANE MACHINERY -- tsp_cutcall.cpp
  int seg_cutcall(int *num_added_p);
  int blossom_cutcall(const int max_cutcount, int *num_added_p);

  //LP HELPER FUNCTIONS -- tsp_lp.cpp
  int basis_init();
  
  bool is_dual_feas();
  bool is_integral();
	
  int primal_opt();
  int no_opt();
	
  int primal_pivot();
  int dual_pivot();
  
  double get_obj_val();
	
  int set_edges();
  int set_support_graph();
  
  void print_lp_edges();
  void print_best_tour_edges();
  void print_best_tour_nodes();

  void printrow(const int rownum, int *rmatind0, int *rmatind1);


  //Member Objects
  Graph &m_graph;
  
  PSEPlp m_lp;
  
  std::vector<double> m_lp_edges;
  
  std::vector<int> old_colstat;
  std::vector<int> old_rowstat;
  
  std::vector<int> support_indices;
  std::vector<int> support_elist;
  std::vector<double> support_ecap;
  
  std::vector<int> best_tour_edges;
  std::vector<int> best_tour_nodes;
  //perm[best_tour_nodes[i]] == i, perm[j] returns index of j in best tour
  std::vector<int> perm;

  
  double m_min_tour_value;
  static bool devex; //TEMPORARY, CHANGE LATER!!!

  double runtime;

  PSEP_Timer T;
  PSEP_Segcuts segments;
  PSEP_2match blossoms;

  G_Utils gu;
  SupportGraph G_s;
  std::vector<int> island;
  std::vector<int> delta;
  std::vector<int> edge_marks;  

};

#endif


