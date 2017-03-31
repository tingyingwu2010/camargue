Camargue
========

This is the README for Camargue, a TSP solver based on primal
cutting plane methods. Camargue tries to move from tour to tour, either
proving that a tour is optimal or finding a better one. Camargue was
developed in tandem with the research in my master's thesis, "Primal
Cutting Plane Methods for the Traveling Salesman Problem".

To get a sense of the
layout of the code, a good starting point would be the documentation
for CMR::Solver. To get an idea of how branching machinery is laid out, look at
derived classes of CMR::ABC::BaseBrancher, or the namespace CMR::ABC.

If you wish to browse the source code manually, I have tried to
document it in a way that keeps header files compact and
readable. Except for class/structure definitions, the
documentation in a `.hpp` is mostly terse one-liners, with detailed
coumentation of function parameters and behaviors in the `.cpp`
files.

This page contains information on installing and using Camargue. You
can also view the GitHub pages version at

https://cstratopoulos.github.io/camargue/index.html

The text is mostly the same, but you will be able to navigate inline
to the Camargue documentation, created with
[Doxygen](http://www.stack.nl/~dimitri/doxygen/).


See below for

- [Installation instructions](#install)
- [Usage info](#usage)


[Installation](#install)
------------

To get started, you can clone the repository with

    git clone https://github.com/cstratopoulos/camargue.git

For users building the code on one of the UW Unix research servers,
from the camargue main directory just do

    cp scripts/Makefile.template.UWlinux Makefile

The compiler and CPLEX directories are already specified, so you just
need to do the steps below concerning symlinks to Concorde, and
installing externals.

Camargue is a C++11 application, so most importantly you will need a
compiler that is compliant with most of the C++11 standard. In fact the code
has been developed and tested with GCC 4.7, which technically does
*not* support the C++11 standard in full. Anything after 4.7 or later
should be fine, and `g++` on a Mac (which is actually an alias of
Apple `clang`) should be fine too; this is the preset option.
Your compiler is specified by editing the `CC` definition in the `Makefile`.
A bit more detail and example options are given there.

Camargue relies heavily on two main external dependencies:
- The TSP solver/combinatorial optimization library
[Concorde](http://www.math.uwaterloo.ca/tsp/concorde/downloads/downloads.htm).

- The linear programming solver [CPLEX
12](http://www-03.ibm.com/software/products/en/ibmilogcpleoptistud)

Both must be installed before proceeding. A Concorde installation with
CPLEX is best, but it should work with QSOPT too. For help, see

- [here](http://www.leandro-coelho.com/installing-concorde-tsp-with-cplex-linux/)
for Leandro Coelho's guide to doing an install with CPLEX; and
- [here](https://qmha.wordpress.com/2015/08/20/installing-concorde-on-mac-os-x/)
for David S. Johnson's guide to installing on Mac OS X with QSOPT; and
- the [Concorde README](http://www.math.uwaterloo.ca/tsp/concorde/DOC/README.html).

After installing Concorde, go into the directory `camargue/externals`
and create a symlink to the `concorde` directory. That is, `concorde`
must point to the folder containing the files `TSP`, `INCLUDE`,
`CUT`, etc.

After installing CPLEX, open the `Makefile` and edit the definitions
`CPXDIR` and `CPX_LIB`. Details and examples are given in the
`Makefile`.

No further edits to the Makefile should be necessary. After that, you
can run the script `cmr_install.sh` to configure and install
Camargue. `cmr_install.sh` accepts flag arguments to configure the
installation to your preferences, and to edit certain other
files. Information on individual external dependences is [here](externals/extdeps.md). The simplest options are `-F` and `-B`. So

    ./cmr_install.sh -F

will run a `F`ull install with all the external dependencies. Assuming
you are connected to the internet, this will download, extract, and
edit all the necessary external dependencies for a full install. This is
recommended for best performance, and to observe all the features
described in my thesis. The complementary option is

    ./cmr_install.sh -B

which runs a `B`are install with nothing other than Concorde and
CPLEX.

In either case, `cmr_install.sh` will then invoke
`configure.sh` to generate a `config.hpp` file and make appropriate
edits to the `Makefile`. You can double check both of these to see if
everything looks right.

If you went with a barebones install and later want to add some or all
external enhancements, you should just be able to invoke
`cmr_install.sh` again. For example to add just the safe Gomory code,
you would do

    ./cmr_install.sh -s

After performing all these steps, you should be able to compile and
run Camargue by running `make` from the main directory, creating the
`camargue` executable.

Additionally, if you like, you can run the unit
tests, benchmarks, and experiments that I used to develop
Camargue (and write my thesis!) by using the recipe `make test`. This
requires that the Catch unit testing framework be
installed, which is done by running `cmr_install.sh` with the `-F`
full install, or with the flag `-c`. Install info is
[here](externals/extdeps.md), with specific
information on invoking the unit tests
[here](source/tests/unittests.md).

[Usage](#usage)
------

This heading is about standard command line usage of Camargue. For
information on running tests/benchmarks, see [their documentation](source/tests/unittests.md).

This section will try to elaborate a bit on the terse documentation
that you get from typing `./camargue` with no arguments.

Camargue accepts problems in two formats: TSPLIB instances with a
`.tsp` suffix, and randomly generated Euclidean instances. If you have
a folder called `problems` in the Camargue directory with TSPLIB
instances in it, you can run one with

    ./camargue problems/dantzig42.tsp

This will attempt to augment or prove the optimality of a starting
tour computed by Concorde's implementation of chained
Lin-Kernighan. You can also specify a starting tour with the '-t' flag:

    ./camargue problems/dantzig42.tsp -t test_data/tours/dantzig42.sol

will run the solver with the starting tour `dantzig42.sol`. The format
of solution files supported is *not* the TSPLIB `.tour` format. Rather,
it should be a file whose first line is the instance node count, with
the following lines (with arbitrary spacing/indentation) giving
zero-indexed ordering of the nodes. If you would like to use a tour in the
TSPLIB `.tour` format, there is a simple script at `scripts/to_sol.sh` that
will convert a `.tour` file to a `.sol` file for you.

When loading a starting tour, Camargue will make sure no obvious
mistakes are present, checking that no node appears twice and that the
indices are drawn from the proper range. You are free to specify an
abysmal starting tour, though!

Random problems can be generated with the flag argument `-R`, and some
additional arguments. To generate a 500-node instance on the 1,000 by
1,000 square grid, run

    ./camargue -Rn500 -g1000

So `-R` is the flag, `-n` specifies node count, and `-g` specifies the
gridsize.

For both styles of problems, Camargue will generate an initial edge
set consisting of the nodes in the tour found, plus the union of 10
quick Lin-Kernighan runs as implemented by Concorde's edge generation
code. For Euclidean-norm instances, the option `-e 1` can be used to
set the Delaunay triangulation edges as the starting edge set too.

Also for both styles of problems, you can pass a random seed with
`-s`. This is to allow reproducibility through all areas of the
code. (Note, however, that if OMP is [enabled](externals/extdeps.md),
non-determinism will still be present.)
For a random problem, this will be used to pick the distribution
of points on the grid. For both types of problems, it will also always
be used in calls to edge generators, separation routines,
etc. Negative arguments, or an argument of zero, will result in the
current time being used.

By default, Camargue will do a loop of pivoting and cutting for as
long as possible, and then begin a so-called Augment-Branch-Cut (ABC)
search. The flag option `-P` will disable branching, attempting a "pure"
primal cutting plane solution method instead.

Camargue implements several different node selection rules for guiding
the ABC search -- these can be specified by passing options to `-b`;
see the usage info from running `./camargue` for more info.

The flag option `-S` is available to specify sparse solution
mode. In this mode, Camargue will run no edge pricing of any kind; it
will just generate an initial edge set as above and try to prove that
a given starting tour is optimal for this edge set. This option is
required for the use of primal safe Gomory cut separation, described
in the [external dependencies](externals/extdeps.md).

Moreover, users can also specify cut generation style with
`-c`. Camargue contains implementations of certain primal separation
algorithms based on the research of Letchford and Lodi, as well as
Fleischer, Letchford and Lodi. The option `-c 0` will select these
algorithms, as well as heuristics for fast blossoms and block
combs. The `-c 1` option, the default, will supplement these with some
more exotic standard separation routines implemented in the Concorde
TSP solver, such as cut tightening, double deckers, path inequalities,
comb teething, and local cuts. Camargue will struggle to solve most
instances with the `-c 0` option, but it can still be used.

Finally, there is the `-l [int or float]` option to specify a target
lower bound for the solver. Since Camargue works by trying to augment
starting tours, you may be satisfied with terminating the solver
prematurely if some target objective value is met. Values supplied
will be rounded up to their integer ceiling, with the interpretation
that integer values represent tour lengths and floating points
represent dual lower bounds.