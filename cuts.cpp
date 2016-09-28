#include "cuts.hpp"


using namespace std;

namespace PSEP {
  
template<>
void CutQueue<HyperGraph>::push_front(const HyperGraph &H)
{
  cut_q.push_front(H);
  if(cut_q.size() > q_capacity){
    cut_q.back().delete_refs();
    cut_q.pop_back();
  }
}

template<>
void CutQueue<HyperGraph>::pop_front()
{
  cut_q.front().delete_refs();
  cut_q.pop_front();
}

}

using namespace PSEP;

int CutTranslate::get_sparse_row(const HyperGraph &H, vector<int> &rmatind,
				 vector<double> &rmatval, char &sense,
				 double &rhs)
{
  int rval = 0;
  sense = 'G';

  vector<int> body_nodes;
  int deltacount = 0;
  
  rval = H.source_setbank->extract_nodelist(*(H.set_refs[0]),
					    body_nodes);
  PSEP_CHECK_RVAL(rval, "Couldn't extract nodelist, ");

  GraphUtils::get_delta(body_nodes, edges, &deltacount, delta, edge_marks);
  rval = (deltacount == 0);
  PSEP_CHECK_RVAL(rval, "Body nodes gave empty delta, ");

  try { rmatind.resize(deltacount); rmatval.resize(deltacount, 1.0); }
  catch(const std::bad_alloc &){
    rval = 1; PSEP_GOTO_CLEANUP("Out of memory for sparse row, ");
  }

  for(int i = 0; i < deltacount; i++)
    rmatind[i] = delta[i];

  switch(H.cut_type){
  case HyperGraph::CutType::Segment:
    rhs = 2;
    goto CLEANUP;
    break;

  case HyperGraph::CutType::Blossom:
    int num_teeth = H.set_refs.size() - 1;
    rhs = 1 - num_teeth;
    
    //skip the first ref, it is body above
    for(int i = 1; i < H.set_refs.size(); i++){
      vector<int> edge_tooth;
      int edge_index;
      IntPairMap::iterator find_it;

      rval = H.source_setbank->extract_nodelist(*(H.set_refs[i]), edge_tooth);
      PSEP_CHECK_RVAL(rval, "Couldn't extract blossom tooth, ");

      if(edge_tooth.size() != 2){
	rval = 1; PSEP_GOTO_CLEANUP("Blossom tooth has " << edge_tooth.size()
				    << "nodes! ");
      }

      find_it = edge_lookup.find(IntPair(fmin(edge_tooth[0], edge_tooth[1]),
					 fmax(edge_tooth[0], edge_tooth[1])));
      rval = (find_it == edge_lookup.end());
      PSEP_CHECK_RVAL(rval, "Couldn't find tooth in edge lookup, ");

      edge_index = find_it->second;
      rmatval[edge_index] = -1.0;
    }
    
    break;
  }

 CLEANUP:
  if(rval){
    cerr << "CutTranslate<HyperGraph>::get_sparse_row failed, row is invalid\n";
    rmatind.clear();
    rmatval.clear();
  }
  return rval;
}
