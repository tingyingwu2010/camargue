#include<iostream>

#include "tsp_solver.hpp"
#include "lp.hpp"
#include "PSEP_util.hpp"

using namespace std;
using namespace PSEP;

TSPSolver::TSPSolver(const string &fname, RandProb &randprob, LP::Prefs _prefs,
		     unique_ptr<CCdatagroup> &dat,
		     const bool sparse) :
  GraphGroup(fname, randprob, dat, sparse),
  BestGroup(GraphGroup.m_graph, GraphGroup.delta, dat),
  LPGroup(GraphGroup.m_graph, _prefs, BestGroup.perm){

  if(!GraphGroup || !BestGroup || !LPGroup){
    cerr << "Bad DataGroup, PureCut will not be constructed\n";
    PureCut.reset(NULL);
    return;
  }

  try{
    PureCut.reset(new PSEP::PureCut(GraphGroup, BestGroup, LPGroup,
				    SupportGroup));
  } catch (const std::bad_alloc &){
    cerr << "TSPSolver constructor failed to construct PureCut\n";
    PureCut.reset(NULL);
  }
}

int TSPSolver::call(SolutionProtocol solmeth, const bool sparse){
  LP::PivType piv_status;
  int rval = 0;

  rval = !PureCut;
  PSEP_CHECK_RVAL(rval, "PureCut is NULL, ");
  
  if(solmeth == SolutionProtocol::PURECUT){
    PivotPlan plan;
    if(sparse)
      plan = PivotPlan(GraphGroup.m_graph.node_count, PivPresets::SPARSE);      
   
    if(PureCut->solve(plan, piv_status)){
      cerr << "TSPSolver.call(PURECUT) failed\n";
      return 1;
    }
    
    return 0;
    
  } else {
    PivotPlan plan(GraphGroup.m_graph.node_count, PivPresets::ROOT);
    if(PureCut->solve(plan, piv_status)){
      cerr << "TSPSolver.call(ABC) failed to call PureCut at root\n";
      return 1;
    }

    if(piv_status == LP::PivType::FathomedTour) return 0;
    if(piv_status == LP::PivType::Subtour){
      cerr << "Terminated with inseparable subtour inequality\n";
      return 1;
    }

    int ecount = GraphGroup.m_graph.edge_count;
    std::vector<double> lower_bounds;
    try{ lower_bounds.resize(ecount); }
    catch(const std::bad_alloc &) {
      rval = 1; PSEP_GOTO_CLEANUP("Couldn't allocate lower bounds, ");
    }

    if(PSEPlp_getlb(&(LPGroup.m_lp), &lower_bounds[0], 0, ecount - 1)){
      cerr << "TSPSolver.call(ABC) failed to get lower bounds\n";
      return 1;
    }

    ABC.reset(new PSEP::ABC(BB::BranchPlan::Main,
			    GraphGroup, BestGroup, LPGroup, SupportGroup,
			    lower_bounds, *PureCut));

    return ABC->solve();
  }

 CLEANUP:
  if(rval)
    cerr << "TSPSolver.call failed\n";
  return rval;
}
  
