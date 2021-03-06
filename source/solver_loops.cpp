/**
 * @file
 * @brief Private Solver methods with loop calls or utility functions.
 */

#include "config.hpp"
#include "solver.hpp"
#include "separator.hpp"

#if CMR_HAVE_SAFEGMI
#include "safeGMI.hpp"
#endif

#include "err_util.hpp"

#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <functional>
#include <vector>

#include <cmath>
#include <cstdio>

using std::abs;
using std::ceil;

using std::function;
using std::vector;
using std::unique_ptr;

using std::cout;
using std::cerr;
using std::endl;

using std::runtime_error;
using std::logic_error;
using std::exception;

namespace CMR {

using CutType = Sep::HyperGraph::Type;
using PivType = LP::PivType;
namespace Eps = Epsilon;

/**
 * This function template is used to add one class of cuts at a time, attempt
 * to measure progress obtained, and return information on whether a further
 * separation routine should be called.
 * @tparam Qtype the queue representation of the cuts found by \p sepcall.
 * @param[in] sepcall a function which returns true if cuts are found, in which
 * case they are stored in \p sep_q.
 * @param[out] piv if cuts are found, this is the pivot type that occurred
 * after they were added.
 * @param[in,out] piv_stats tracker for the pivot objective values seen this
 * round.
 */
template<typename Qtype>
bool Solver::call_separator(const function<bool()> &sepcall, Qtype &sep_q,
                            const std::string sep_name,
                            PivType &piv, PivStats &piv_stats,
                            bool pivback_prune)
{
    TimerCalled &tpair = sep_times[sep_name];
    Timer &sep_timer = tpair.first;
    tpair.second = true;

    sep_timer.resume();
    bool result = sepcall();
    sep_timer.stop();

    if (result) {
        time_piv.resume();
        core_lp.pivot_back(pivback_prune);
        core_lp.add_cuts(sep_q);
        piv = core_lp.primal_pivot();
        time_piv.stop();

        double prev_val = piv_stats.prev_val;
        double new_val = core_lp.get_objval();
        double tourlen = core_lp.active_tourlen();

        piv_stats.update(new_val, tourlen);
        piv_stats.found_cuts = true;

        if (output_prefs.prog_bar)
            place_pivot(0.99 * tourlen, tourlen, new_val);

        if (output_prefs.verbose) {
            printf("\t^^Pivot %.2f -> %.2f, ratio %.2f\n",
                   prev_val, new_val, piv_stats.delta_ratio);
            piv_stats.report_extrema();
        }
    }

    return result;
}

double Solver::PH_delta(double new_val, double prev_val, double tourlen)
{
    return std::abs((new_val - prev_val) / (tourlen - prev_val));
}

void Solver::PivStats::update(double new_val, double tourlen)
{
    delta_ratio = Solver::PH_delta(new_val, prev_val, tourlen);
    first_last_ratio = Solver::PH_delta(new_val, initial_piv, tourlen);
    prev_val = new_val;

    if (delta_ratio > max_ratio)
        max_ratio = delta_ratio;

    if (prev_val < lowest_piv)
        lowest_piv = prev_val;
}

void Solver::PivStats::report_extrema()
{
    printf("\tLowest piv %.2f\tMax ratio %.2f\tFirst-last ratio %.2f\n",
           lowest_piv, max_ratio, first_last_ratio);
    cout << std::flush;
}

/**
 * @param piv the PivType of the last call to primal_pivot.
 * @param delta_metric some measurement of the change obtained from the last
 * round of cut generation.
 */
bool Solver::restart_loop(LP::PivType piv, double delta_metric)
{
    return (piv == PivType::Subtour || !core_lp.supp_data.connected ||
            delta_metric >= Eps::PHratio);
}

inline bool Solver::return_pivot(LP::PivType piv)
{
    return (piv == PivType::Tour || piv == PivType::FathomedTour);
}

inline void Solver::place_pivot(double low_lim, double best_tourlen,
                                double piv_val)
{
    int target_entry = 1;
    char piv_char = '>';
    if (piv_val <= low_lim)
        piv_char = '!';
    else if (piv_val == best_tourlen) {
        piv_char = '*';
        target_entry = 78;
    } else {
        target_entry = ceil(78 * ((piv_val - low_lim) /
                                  (best_tourlen - low_lim)));
    }

    p_bar[target_entry] = piv_char;

    int i = 0;
    cout << p_bar[i];
    for (i = 1; i < target_entry; ++i)
        cout << '#';
    while (i < 80)
        cout << p_bar[i++];

    cout << "\r" << std::flush;
    p_bar[target_entry] = ' ';
}

/**@name Convenience macros for separation routines in Solver::cut_and_piv.
 * These macros are meant to be used in the body of the main `while` loop
 * in Solver::cut_and_piv to invoke a separation routine using
 * Solver::call_separator, and then restart or advance the loop as needed.
 * This saves the use of repetitive boilerplate which can make it hard to
 * navigate the loop at a glance, given the `return` and `continue` statements
 * which may not be easy to abstract into a real function template.
 */
///@{

/** Macro for invoking a conventional separation routine.
 * In the body of Solver::cut_and_piv, this can be used to
 * invoke a straightforward separation routine which is just called and used
 * to update stats. See below for example usage.
 */
#define CUT_PIV_CALL(sep_ptr, find_fn, cuts_q, sep_name)                \
try {                                                                   \
    reset_separator(sep_ptr);                                           \
    if (call_separator([&sep_ptr, this]()                               \
                       { return sep_ptr->find_fn; },                    \
                       sep_ptr->cuts_q(), sep_name, piv, piv_stats,     \
                       true)) {                                         \
        if (return_pivot(piv))                                          \
            return piv;                                                 \
                                                                        \
        if (restart_loop(piv, piv_stats.delta_ratio))                   \
            continue;                                                   \
    }                                                                   \
} CMR_CATCH_PRINT_THROW(sep_name, err);

/** Macro for invoking cut metamorphosis separation.
 * This can be used for a straightforward invocation of one of the
 * Sep::MetaCuts separation routines. See below for example usage.
 */
#define META_SEP_PIV_CALL(meta_type, sep_name)                          \
try {                                                                   \
    reset_separator(meta_sep);                                          \
    if (call_separator([&meta_sep]()                                    \
                       { return meta_sep->find_cuts(meta_type); },      \
                       meta_sep->metacuts_q(), sep_name, piv,           \
                       piv_stats, true)) {                              \
        if (return_pivot(piv))                                          \
            return piv;                                                 \
                                                                        \
        if (restart_loop(piv, piv_stats.delta_ratio))                   \
            continue;                                                   \
    }                                                                   \
}  CMR_CATCH_PRINT_THROW(sep_name, err);

#define STD_SEC_PIV_LOOP(sec_name, sec_call, sec_q)                     \
if (verbose)                                                            \
    cout << "\tAdding round of " << sec_name << " SECs...\n";           \
                                                                        \
int numrounds = 0;                                                      \
bool found_cuts = true;                                                 \
                                                                        \
while (found_cuts) {                                                    \
    ++numrounds;                                                        \
    try {                                                               \
        reset_separator(sep);                                           \
        found_cuts = call_separator([&sep]() { return sep->sec_call(); }, \
                                    sep->sec_q(), sec_name, piv, piv_stats, \
                                    false);                             \
    } CMR_CATCH_PRINT_THROW(sec_name, err);                             \
}                                                                       \
                                                                        \
if (return_pivot(piv)) {                                                \
    if (verbose)                                                        \
        cout << "\t...Tour pivot from " << numrounds << " rounds of "   \
             << sec_name << " SECs" << endl;                            \
    return piv;                                                         \
} else {                                                                \
    if (verbose)                                                        \
        cout << "\t...Generated " << sec_name << " SECs for " << numrounds \
             << " rounds, none remain" << endl;                         \
}



///@}

/**
 * @param do_price is edge pricing being performed for this instance. Used to
 * control Gomory cut separation.
 * @returns a LP::PivType corresponding to a non-degenerate pivot that was
 * obtainedfrom repeated rounds of cut generation and pivoting.
 */
PivType Solver::cut_and_piv(bool do_price)
{
    runtime_error err("Problem in Solver::cut_and_piv");

    PivType piv = PivType::Frac;
    int round = 0;
    bool &verbose = output_prefs.verbose;

    Data::SupportGroup &supp_data = core_lp.supp_data;

    unique_ptr<Sep::Separator> sep;
    unique_ptr<Sep::MetaCuts> meta_sep;

    if (output_prefs.prog_bar) {
        std::fill(p_bar.begin(), p_bar.end(), ' ');
        p_bar[0] = '[';
        p_bar[79] = ']';
    }

    while (true) {
        try { piv = core_lp.primal_pivot(); }
        CMR_CATCH_PRINT_THROW("initializing pivot and separator", err);

        if (return_pivot(piv))
            return piv;

        PivStats piv_stats(core_lp.get_objval());
        double &delta_ratio = piv_stats.delta_ratio;
        bool called_ptight = false;

        ++round;
        if (verbose) {
            printf("Round %d, first piv %.2f, ",
                   round, core_lp.get_objval());
            cout << piv << ", " << core_lp.num_rows() << " rows, "
                 << core_lp.num_cols() << " cols" << endl;
        }

        if (cut_sel.cutpool)
            CUT_PIV_CALL(sep, pool_sep(core_lp.ext_cuts), cutpool_q,
                         "CutPool");

        if (cut_sel.segment)
            CUT_PIV_CALL(sep, segment_sep(), segment_q, "SegmentCuts");

        if (cut_sel.connect && !supp_data.connected) {
            STD_SEC_PIV_LOOP("ConnectCuts", connect_sep, connect_cuts_q);
            continue;
        }

        if (cut_sel.fast2m)
            CUT_PIV_CALL(sep, fast2m_sep(), fastblossom_q, "FastBlossoms");

        if (cut_sel.blkcomb)
            CUT_PIV_CALL(sep, blkcomb_sep(), blockcomb_q, "BlockCombs");

        if (cut_sel.ex2m)
            CUT_PIV_CALL(sep, exact2m_sep(), exblossom_q, "ExactBlossoms");

        if (cut_sel.simpleDP) {
            STD_SEC_PIV_LOOP("ExactSub", exsub_sep, exact_sub_q);
            CUT_PIV_CALL(sep, simpleDP_sep(), simpleDP_q, "SimpleDP");
        }

        using MetaType = Sep::MetaCuts::Type;

        if (cut_sel.tighten)
            CUT_PIV_CALL(meta_sep, tighten_cuts(), metacuts_q, "Tighten");

        if (cut_sel.decker)
            META_SEP_PIV_CALL(MetaType::Decker, "Decker");

        if (cut_sel.handling)
            META_SEP_PIV_CALL(MetaType::Handling, "Handling");

        if (cut_sel.teething)
            META_SEP_PIV_CALL(MetaType::Teething, "Teething");

        if (cut_sel.tighten_pool && !called_ptight) {
            //called_ptight = true;
            CUT_PIV_CALL(sep, tighten_pool(core_lp.ext_cuts), cutpool_q,
                         "TightenPool");
        }

        if (cut_sel.consec1)
            CUT_PIV_CALL(sep, consec1_sep(core_lp.ext_cuts), consec1_q,
                         "Consec1");

        if (cut_sel.localcuts) {
            bool lc_restart = false;
            for (int chk = 8; chk <= Sep::LocalCuts::MaxChunkSize; ++chk) {
                reset_separator(sep);
                bool do_sphere = false;

                if (call_separator([&sep, chk, do_sphere]()
                                   { return sep->local_sep(chk, do_sphere); },
                                   sep->local_cuts_q(), "LocalCuts", piv,
                                   piv_stats, true)) {
                    if (return_pivot(piv))
                        return piv;

                    lc_restart = restart_loop(piv, delta_ratio);
                    if (lc_restart)
                        break;
                }
            }

            if (lc_restart)
                continue;

            for (int chk = 8; chk <= Sep::LocalCuts::MaxChunkSize; ++chk) {
                reset_separator(sep);
                bool do_sphere = true;

                if (call_separator([&sep, chk, do_sphere]()
                                   { return sep->local_sep(chk, do_sphere); },
                                   sep->local_cuts_q(), "LocalCuts",
                                   piv, piv_stats, true)) {
                    if (return_pivot(piv))
                        return piv;

                    lc_restart = restart_loop(piv, delta_ratio);
                    if (lc_restart)
                        break;
                }
            }

            if (lc_restart)
                continue;
        }

#if CMR_HAVE_SAFEGMI

        unique_ptr<Sep::SafeGomory> gmi_sep;

        if (cut_sel.safeGMI && !do_price)
            CUT_PIV_CALL(gmi_sep, find_cuts(), gomory_q, "safeGMI");

#endif
        double ph_init_prev = piv_stats.first_last_ratio;
        bool found_cuts = piv_stats.found_cuts;

        if (found_cuts && ph_init_prev >= 0.01)
            continue;

        if (output_prefs.prog_bar)
            cout << endl;

        if (verbose) {
            cout << "Tried all routines, found_cuts " << found_cuts
                 << ", returning " << piv << endl;
            piv_stats.report_extrema();
        }

        break;
    }

    return piv;
}

void Solver::opt_check_prunable(bool do_price, ABC::BranchNode &prob)
{
    runtime_error err("Problem in Solver::opt_check_prunable");

    using BranchStat = ABC::BranchNode::Status;
    int verbose = output_prefs.verbose;

    cout << "Price check: " << ABC::bnode_brief(prob) << " based on ";

    double opt_time = util::zeit();
    try {
        if (prob.price_basis && !prob.price_basis->empty()) {
            cout << "optimal estimate";
            core_lp.copy_base(prob.price_basis->colstat,
                              prob.price_basis->rowstat);
        } else {
            cout << "high estimate";
            core_lp.copy_start(core_lp.active_tour.edges());
        }
        cout << endl;

        unique_ptr<Sep::Separator> sep;
        reset_separator(sep);

        sep_times["CutPool"].first.resume();
        if (sep->pool_tour_tight(core_lp.ext_cuts))
            core_lp.add_cuts(sep->cutpool_q());
        sep_times["CutPool"].first.stop();

        core_lp.primal_opt();
    } CMR_CATCH_PRINT_THROW("pool adding/optimizing for pricing", err);
    opt_time = util::zeit() - opt_time;

    double objval = core_lp.get_objval();

    if (verbose) {
        printf("\tPrimal opt objval %.2f in %.2fs", objval, opt_time);
        cout << endl;
    }

    prob.stat = BranchStat::NeedsCut;

    if (objval >= core_lp.global_ub() - 0.9) {
        if (!do_price) {
            cout << "\tSparse problem prunable." << endl;
            prob.stat = BranchStat::Pruned;
        } else {
            Price::ScanStat pstat = Price::ScanStat::Full;
            try {
                time_price.resume();
                pstat = edge_pricer->gen_edges(PivType::FathomedTour,
                                               false);
                time_price.stop();
            } CMR_CATCH_PRINT_THROW("pricing edges", err);

            if (pstat == Price::ScanStat::FullOpt) {
                prob.stat = BranchStat::Pruned;
                cout << "Problem pruned by LB, do not cut." << endl;
            }
        }
    }

}

PivType Solver::abc_bcp(bool do_price)
{
    using BranchStat = ABC::BranchNode::Status;
    using ABC::BranchHistory;

    runtime_error err("Problem in Solver::abc_bcp");

    PivType piv = PivType::Frac;
    BranchHistory::iterator cur = branch_controller->next_prob();

    while (cur != branch_controller->get_history().end()) {
        cout << "\n";

        if (cur->stat == BranchStat::NeedsRecover) {
            cout << ABC::bnode_brief(*cur) << " needs feas recover"
                 << endl;
            if (do_price) {
                bool feasible = false;

                try {
                    time_price.resume();
                    feasible = edge_pricer->feas_recover();
                    time_price.stop();
                }
                CMR_CATCH_PRINT_THROW("doing feas_recover on node", err);

                if (feasible) {
                    cout << "Recovered infeasible BranchNode." << endl;
                    cur->stat = BranchStat::NeedsCut;
                } else {
                    cout << "Pricing pruned node by infeasibility." << endl;
                    cur->stat = BranchStat::Pruned;
                }
            } else {
                cout << "Infeasible sparse problem can be pruned" << endl;
                cur->stat = BranchStat::Pruned;
            }
        }

        if (cur->stat != BranchStat::Pruned)
            try {
                time_branch.resume();
                branch_controller->do_branch(*cur);
                time_branch.stop();
                if (core_lp.active_tourlen() < best_data.min_tour_value) {
                    cout << "Instated branch tour improves on best tour"
                         << endl;
                    core_lp.active_tour.best_update(best_data);
                    report_aug(Aug::Branch);
                    if (lb_fathom()) {
                        cout << "Branch tour is optimal by LB, "
                             << "terminating ABC search." << endl;
                        return PivType::FathomedTour;
                    }
                }
            } CMR_CATCH_PRINT_THROW("branching on current problem", err);

        if (cur->stat == BranchStat::NeedsPrice)
            try {
                time_price.resume();
                opt_check_prunable(do_price, *cur);
                time_price.stop();
            } CMR_CATCH_PRINT_THROW("doing opt price check", err);

        if (cur->stat == BranchStat::NeedsCut) {
            cout << "Cutting on " << ABC::bnode_brief(*cur) << endl;
            try {
                piv = cutting_loop(do_price, false, false);
            } CMR_CATCH_PRINT_THROW("cutting branch prob", err);

            if (piv == PivType::Frac)
                cur->stat = BranchStat::NeedsBranch;
            else if (piv == PivType::FathomedTour) {
                cur->stat = BranchStat::Pruned;
                printf("\tPruned with opt objval %.2f", core_lp.get_objval());
                cout << endl;
                if (lb_fathom()) {
                    cout << "Terminating ABC search by lower bound." << endl;
                    return piv;
                }
            } else {
                cerr << "Pivot status " << piv << " in abc" << endl;
                throw err;
            }
        }

        if (cur->stat == BranchStat::NeedsBranch) {
            cout << "Branching on " << ABC::bnode_brief(*cur) << endl;
            try {
                time_branch.resume();
                branch_controller->split_prob(cur);
                time_branch.stop();
            } CMR_CATCH_PRINT_THROW("splitting branch problem", err);

            cur->stat = BranchStat::Done;
        }

        time_branch.resume();
        try { branch_controller->do_unbranch(*cur); }
        CMR_CATCH_PRINT_THROW("unbranching pruned problem", err);

        cur = branch_controller->next_prob();
        time_branch.stop();
    }

    return piv;
}

PivType Solver::frac_recover()
{
    runtime_error err("Problem in Solver::frac_recover");

    Data::SupportGroup &s_dat = core_lp.supp_data;
    int ncount = tsp_instance.node_count();
    vector<int> cyc;

    try { cyc.resize(ncount); } CMR_CATCH_PRINT_THROW("allocating cyc", err);

    double val = std::numeric_limits<double>::max();
    CCrandstate rstate;
    CCutil_sprand(tsp_instance.seed(), &rstate);

    if (CCtsp_x_greedy_tour_lk(tsp_instance.ptr(), ncount,
                               s_dat.support_ecap.size(),
                               &s_dat.support_elist[0],
                               &s_dat.support_ecap[0], &cyc[0], &val, true,
                               &rstate)) {
        cerr << "CCtsp_x_greedy_tour_lk failed.\n";
        throw err;
    }

    if (val >= core_lp.active_tourlen())
        return PivType::Frac;

    vector<Graph::Edge> new_edges;

    for (int i = 0; i < ncount; ++i) {
        EndPts e(cyc[i], cyc[(i + 1) % ncount]);
        if (core_graph.find_edge_ind(e.end[0], e.end[1]) == -1) {
            try {
                new_edges.emplace_back(e.end[0], e.end[1],
                                       tsp_instance.edgelen(e.end[0],
                                                            e.end[1]));
            } CMR_CATCH_PRINT_THROW("emplacing new edge", err);
        }
    }

    if (!new_edges.empty()) {
        try {
            core_lp.add_edges(new_edges, false);
        } CMR_CATCH_PRINT_THROW("adding edges not in tour", err);
    }

    if (output_prefs.verbose)
        cout << "Recovered to tour of length " << val << ", "
             << new_edges.size() << " new edges added" << endl;

    try { core_lp.set_active_tour(std::move(cyc)); }
    CMR_CATCH_PRINT_THROW("passing recover tour to core_lp", err);

    return LP::PivType::Tour;
}

void Solver::reset_separator(unique_ptr<Sep::Separator> &S)
{
    util::ptr_reset(S, core_graph.get_edges(), active_tour(),
                    core_lp.supp_data, karp_part,
                    tsp_instance.seed());
    S->filter_primal = !active_tour().tourless();
    S->verbose = output_prefs.verbose;
}

void Solver::reset_separator(std::unique_ptr<Sep::MetaCuts> &MS)
{
    util::ptr_reset(MS, core_lp.external_cuts(), graph_info().get_edges(),
                    active_tour(), core_lp.supp_data);
    MS->filter_primal = !active_tour().tourless();
    MS->verbose = output_prefs.verbose;
}

void Solver::reset_separator(std::unique_ptr<Sep::SafeGomory> &GS)
#if CMR_HAVE_SAFEGMI
{
    util::ptr_reset(GS, core_lp, active_tour().edges(),
                    core_lp.lp_edges);
    GS->filter_primal = !active_tour().tourless();
    GS->verbose = output_prefs.verbose;
}
#else
{
    throw logic_error("called gmi reset_separator but CMR_HAVE_SAFEGMI undef'd")
}
#endif

}
