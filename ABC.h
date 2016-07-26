#ifndef PSEP_ABC_H
#define PSEP_ABC_H

#include<memory>
#include<stack>

#include "BButils.h"
#include "datagroups.h"
#include "cutcall.h"

class PSEP_ABC {
 public:
  PSEP_ABC(PSEP_GraphGroup &GraphGroup, PSEP_BestGroup &BestGroup,
	   PSEP_LPGroup &LPGroup, PSEP_SupportGroup &SupportGroup):
  cutcall(GraphGroup, BestGroup, LPGroup, SupportGroup),
    LPcore(LPGroup, GraphGroup, SupportGroup, BestGroup) {}
  
  int solve();
  
  int previsit(std::unique_ptr<PSEP_BBNode> &v);
  int postvisit(std::unique_ptr<PSEP_BBNode> &v);
  int enforce(std::unique_ptr<PSEP_BBNode> &v);
  int unenforce(std::unique_ptr<PSEP_BBNode> &v);

 private:
  PSEP_Cutcall cutcall;
  PSEP_LP_Core LPcore;
  std::stack<std::unique_ptr<PSEP_BBNode> > BBtree;
};

#endif