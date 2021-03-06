/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/** @file
 * @brief Primal safe Gomory cut separation.
 *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef CMR_SAFEGMI_H
#define CMR_SAFEGMI_H

#include "config.hpp"

#if CMR_HAVE_SAFEGMI


#include "lp_interface.hpp"
#include "mirgroup.hpp"
#include "process_cuts.hpp"

#include <vector>


namespace CMR {
namespace Sep {

/// Primal separation of safe Gomory cuts.
class SafeGomory {
public:
    SafeGomory(LP::Relaxation &rel,
               const std::vector<double>  &_tour_edges,
               const std::vector<double> &lp_x);

    bool find_cuts();
    CutQueue<LP::SparseRow> &gomory_q() { return gmi_q; }

    bool verbose = false;
    bool filter_primal = true;

private:
    LP::Relaxation &lp_relax;
    MIRgroup mir_data;
    CutQueue<LP::SparseRow> gmi_q;

    const std::vector<double> &tour_edges;
    const std::vector<double> &frac_x;
};

}
}

#endif //CMR_HAVE_SAFEGMI
#endif //CMR_SAFEGMI_H
