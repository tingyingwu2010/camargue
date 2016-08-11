#include "cutcall.h"

using namespace std;
using namespace PSEP;

int CutControl::primal_sep(const int augrounds, const LP::PivType stat){
  int rval = 0;
  
  int segval = 2, matchval = 2, dpval = 2;
  double segtime, matchtime, dptime;

  segtime = PSEP_zeit();
  segval = segments.cutcall();
  if(segval == 1){
    rval = 1;
    goto CLEANUP;
  }
  segtime = PSEP_zeit() - segtime;
  total_segtime += segtime;
  total_segcalls++;

  matchtime = PSEP_zeit();
  matchval = blossoms.cutcall();
  if(matchval == 1){
    rval = 1;
    goto CLEANUP;
  }
  matchtime = PSEP_zeit() - matchtime;
  total_2mtime += matchtime;
  total_2mcalls++;

  if(prefs.dp_threshold >= 0 && augrounds >= prefs.dp_threshold){
    if(segval == 2 && matchval == 2 && stat != LP::PivType::Subtour){
      dptime = PSEP_zeit();
      dpval = dominos.cutcall();
      if(dpval == 1){
	rval = 1;
	goto CLEANUP;
      }
      dptime = PSEP_zeit() - dptime;
      total_dptime += dptime;	
    }
  }

  if(segval == 2 && matchval == 2 && dpval == 2)
    rval = 2;

 CLEANUP:
  if(rval == 1)
    cerr << "Problem in CutControl::primal_sep: ";
  if(segval == 1)
    cerr << "Cuts<seg>::cutcall\n";
  if(matchval == 1)
    cerr << "Cuts<blossom>::cutcall\n";
  if(dpval == 1)
    cerr << "Cuts<domino>::cutcall\n";
  return rval;
}

int CutControl::general_sep(const double piv_val){
  int rval = general_cuts.separate(piv_val);
  if(rval == 1)
    cerr << "Problem in CutControl::general_sep\n";
  return rval;
}
