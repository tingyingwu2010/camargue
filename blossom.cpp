#include "blossom.h"

using namespace std;

int PSEP_2match::separate(const int max_cutcount){
  int rval = 0;
  int cut_edge_index;
  int end0, end1;
  int best_tour_entry;
  bool contains_cutedge;
  int ncount = 2 * support_indices.size();
  double orig_weight, changed_weight, cutval;
  int *cut_nodes = (int *) NULL;
  int cutcount = 0;

  cut_ecap.resize(support_ecap.size());

  for(int i = 0; i < support_indices.size(); i++){
    if(best_tour_edges[support_indices[i]] == 0)
      cut_ecap[i] = support_ecap[i];
    else
      cut_ecap[i] = 1 - support_ecap[i];
  }

  for(int i = 0; i < support_indices.size(); i++){
    cut_edge_index = support_indices[i];
    best_tour_entry = best_tour_edges[cut_edge_index];

    switch(best_tour_entry){
    case(0):
      orig_weight = support_ecap[i];
      changed_weight = 1 - support_ecap[i];
      contains_cutedge = true;
      break;
    case(1):
      orig_weight = 1 - support_ecap[i];
      changed_weight = support_ecap[i];
      contains_cutedge = false;
    }

    cut_ecap[i] = changed_weight;

    end0 = support_elist[2 * i];
    end1 = support_elist[(2 * i) + 1];

    rval = CCcut_mincut_st(ncount, support_indices.size(),
			   &support_elist[0], &cut_ecap[0], end0, end1,
			   &cutval, &cut_nodes, &cutcount);
    if(rval){
      cerr << "Problem in 2match::separate w st-cut" << endl;
      goto CLEANUP;
    }

    if(cutval < 1 - LP_EPSILON){      
      vector<int> handle;
      for(int j = 0; j < cutcount; j++){
	handle.push_back(cut_nodes[j]);
      }

      blossom new_2m(handle, cut_edge_index, cutval, contains_cutedge);
      pq.push(new_2m);
    }

    cut_ecap[i] = orig_weight;
  }

  cout << "Total number of blossoms found: " << pq.size() << endl;

 CLEANUP:
  if(cut_nodes) free(cut_nodes);
  return rval;
}

int PSEP_2match::add_cut(const int deltacount, vector<int> &delta,
			 const int cutedge, const bool contains_cutedge){
  int rval = 0, newrows = 1, newnz = deltacount;
  int rmatbeg[1] = {0};
  char sense[1] = {'G'};
  double rhs[1];
  int num_teeth = 0;
  vector<double> rmatval(deltacount, 1.0);
  cout << "All delta coefficients intialized to 1.0" << endl;

  cout << "Setting tooth coefficients..." << endl;
  if(contains_cutedge){
    for(int i = 0; i < deltacount; i++){
      if(best_tour_edges[delta[i]] == 1 ||
	 delta[i] == cutedge){
	  rmatval[i] = -1.0;
	  num_teeth++;
	  cout << "Edge " << delta[i] << " is a tooth, coeff -1.0" << endl;
      }
    }
  } else {
    for(int i = 0; i < deltacount; i++){
      if(best_tour_edges[delta[i]] == 1 &&
	 delta[i] != cutedge){
	rmatval[i] = -1.0;
	num_teeth++;
	cout << "Edge " << delta[i] << " is a tooth, coeff -1.0" << endl;
      }
    }
  }

  double lhs = 0;
  for(int i = 0; i < support_indices.size(); i++){
    for(int j = 0; j < deltacount; j++){
      if(support_indices[i] == delta[j])
	lhs += support_ecap[i] * rmatval[j];
    }
  }

  cout << "Number of teeth: " << num_teeth;
  rhs[0] = 1 - num_teeth;
  
  cout << ", rhs: " << rhs[0] << ", lhs: " << lhs << endl;

  cout << "Is the cut violated: ";
  if(lhs < rhs[0]){
    cout << endl << "!!!!!!!!!!!!" << endl;
    cout << "!!!!YES!!!!!" << endl;
    cout << "!!!!!!!!!!!!" << endl;
  }
  else
    cout << "no." << endl;
  

  rval = PSEPlp_addrows (&m_lp, newrows, newnz, rhs, sense, rmatbeg,
			 &delta[0], &rmatval[0]);

  if(rval)
    cerr << "Entry point: PSEP_2match::add_cut" << endl;
  return rval;
}
