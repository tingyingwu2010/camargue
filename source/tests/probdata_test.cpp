#include "tests.hpp"
#include "datagroups.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>

#include <catch.hpp>


using std::cout;
using std::setw;
using std::vector;
using std::string;
using std::to_string;
using std::pair;

#ifdef PSEP_DO_TESTS

using probpair = pair<string, int>;
using randpair = pair<int, int>;

SCENARIO("Instance constructors throw when they fail",
	 "[!shouldfail][.Data][.Instance]"){
  GIVEN("An input filename that doesn't exist"){
    THEN("The Instance TSPLIB constructor should throw"){
      int ncount = 0;
      REQUIRE_NOTHROW(PSEP::Data::Instance inst("dksaljfkla", ncount));
    }
  }

  GIVEN("Bad random problem data"){
    int ncount = 100, gridsize = 100, seed = 99;
    WHEN("Node count is zero"){
      ncount = 0;
      THEN("The Instance constructor should throw"){
	REQUIRE_NOTHROW(PSEP::Data::Instance inst(seed, ncount, gridsize));
      }
      AND_WHEN("Gridsize is zero"){
	gridsize = 0;
	THEN("The Instance constructor should throw"){
	  REQUIRE_NOTHROW(PSEP::Data::Instance inst(seed, ncount, gridsize));
	}
      }
    }
  }
}

SCENARIO("Instance constructors initialize data as expected",
	 "[Data][Instance]"){
  vector<probpair> tests{probpair("dantzig42", 42),
			probpair("d493", 493),
			probpair("pr1002", 1002)};
  for(probpair &prob : tests){
    string pname = prob.first;
    int expect_ncount = prob.second;
    string pfile = "problems/" + pname + ".tsp";
    GIVEN("The TSPLIB instance " + pname){
      THEN("Instance constructor works and gets " +
	   to_string(expect_ncount) + " nodes"){
	std::unique_ptr<PSEP::Data::Instance> inst;
	int ncount;

	REQUIRE_NOTHROW(inst =
			PSEP::make_unique<PSEP::Data::Instance>(pfile,
								ncount));
	REQUIRE(ncount == expect_ncount);
      }
    }
  }

  vector<randpair> nodes_grid{randpair(100, 1000), randpair(1000, 1000000),
			      randpair(10000, 1000000)};
  for(randpair &prob : nodes_grid){
    GIVEN("A " + to_string(prob.first) + " node instance on a " +
	  to_string(prob.second) + " squared grid"){
      THEN("The Instance constructor works"){
	std::unique_ptr<PSEP::Data::Instance> inst;
	int ncount = prob.first;
	int gsize = prob.second;
	REQUIRE_NOTHROW(inst =
			PSEP::make_unique<PSEP::Data::Instance>(99,
							       ncount,
							       gsize));
      }
    }
  }
}

SCENARIO("Instance constructors copy and move as expected with no leaking",
	 "[Data][Instance][valgrind]"){
  //PSEP::Data::Instance instcpy = inst; compiler error, copy construction!
  
  GIVEN("A normally constructed instance"){
    PSEP::Data::Instance inst(99, 100, 1000);
    THEN("We can make a reference of it"){
      REQUIRE_NOTHROW(PSEP::Data::Instance &instref = inst);
    }
  }

  GIVEN("Another normally constructed instance"){
    PSEP::Data::Instance inst(99, 1000, 1000000);
    THEN("We can move construct it, transferring ownership"){
      REQUIRE_NOTHROW(PSEP::Data::Instance intmv = std::move(inst));
    }
  }
}

#endif