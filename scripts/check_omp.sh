# OpenMP Configuration routine for Camargue
# The script in this file will attempt to check if the user-specified compiler
# supports OpenMP (OMP) parallelism. It makes no changes to the config file
# itself; that is done by config_externals.sh which invokes this as a
# subroutine.
#
# The approach is to check the Makefile for the C++ compiler, and attempt to
# use it to compile a dummy temporary program with the openmp flag added. An
# exit[0] compilation will be used to indicate that the compiler supports OMP.

CC=$(grep 'CC *:=' Makefile | sed 's/.*:= //')
echo Compiler is "$CC"

"$CC" --version 2>&1 > /dev/null

if [ "$?" -ne 0 ]
then
    echo Basic compiler test failed
    exit 1
fi

echo '#include <iostream>
      #include <omp.h>
 int main(void) {
     std::cout << "Compiler supports OMP, max thread count "
     	       << omp_get_max_threads() << "\\n";
}' > tmp_prog.cpp

"$CC" -fopenmp tmp_prog.cpp -o tmp_prog.o && ./tmp_prog.o

worked="$?"

rm tmp_prog*

exit "$worked"