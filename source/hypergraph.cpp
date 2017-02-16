#include "hypergraph.hpp"
#include "err_util.hpp"

#include <algorithm>
#include <iostream>
#include <map>
#include <stdexcept>
#include <utility>

#include <cmath>

using std::unordered_map;
using std::vector;
using std::pair;

using std::cout;
using std::cerr;

using std::runtime_error;
using std::logic_error;
using std::exception;

using lpcut_in = CCtsp_lpcut_in;
using lpclique = CCtsp_lpclique;

namespace CMR {
namespace Sep {


/**
 * The cut corresponding to \p cc_lpcut will be represented using
 * Clique pointers from \p bank, assuming that the cut was found with
 * \p tour as the resident best tour.
 */
HyperGraph::HyperGraph(CliqueBank &bank, const lpcut_in &cc_lpcut,
                       const vector<int> &tour) try :
    sense(cc_lpcut.sense), rhs(cc_lpcut.rhs), source_bank(&bank),
    source_toothbank(nullptr)
{
    for (int i = 0; i < cc_lpcut.cliquecount; ++i) {
        lpclique &cc_clq = cc_lpcut.cliques[i];
        cliques.push_back(source_bank->add_clique(cc_clq, tour));
    }

} catch (const exception &e) {
    cerr << e.what() << "\n";
    throw runtime_error("HyperGraph CC lpcut_in constructor failed.");
}

/**
 * The cut corresponding to \p dp_cut will be represented using Clique
 * pointers from \p bank and Tooth pointers from \p tbank, assuming the cut
 * was found with \p tour as the resident best tour. The righthand side of the
 * cut stored shall be \p _rhs.
 */
HyperGraph::HyperGraph(CliqueBank &bank, ToothBank &tbank,
                       const dominoparity &dp_cut, const double _rhs,
                       const std::vector<int> &tour) try :
    sense('L'), rhs(_rhs), source_bank(&bank), source_toothbank(&tbank)
{
    vector<int> nodes(dp_cut.degree_nodes);
    for(int &n : nodes)
        n = tour[n];

    cliques.push_back(source_bank->add_clique(nodes));

    for (const SimpleTooth &T : dp_cut.used_teeth)
        teeth.push_back(source_toothbank->add_tooth(T, tour));

} catch (const exception &e) {
    cerr << e.what() << "\n";
    throw runtime_error("HyperGraph dominoparity constructor failed.");
}

/**
 * Constructs a HyperGraph from the handle nodes and edge teeth nodes of an
 * exact primal blossom inequality.
 * @param[in] bank the source CliqueBank.
 * @param[in] blossom_handle the handle of the blossom inequality;
 * @param[in] tooth_edges a vector of vectors of length two, consisting of
 * the end points of edges which are the blossom teeth.
 */
HyperGraph::HyperGraph(CliqueBank &bank,
                       const vector<int> &blossom_handle,
                       const vector<vector<int>> &tooth_edges) try
    : sense('G'), rhs ((3 * tooth_edges.size()) + 1), source_bank(&bank),
      source_toothbank(nullptr)
{
    vector<int> handle(blossom_handle);
    cliques.push_back(source_bank->add_clique(handle));

    for (vector<int> tooth_edge : tooth_edges)
        cliques.push_back(source_bank->add_clique(tooth_edge));
} catch (const exception &e) {
    cerr << e.what() << "\n";
    throw runtime_error("HyperGraph ex_blossom constructor failed.");
}

/**
 * The moved-from Hypergraph \p H is left in a null but valid state, as if it
 * had been default constructed.
 */
HyperGraph::HyperGraph(HyperGraph &&H) noexcept
    : sense(H.sense), rhs(H.rhs),
      cliques(std::move(H.cliques)), teeth(std::move(H.teeth)),
      source_bank(H.source_bank), source_toothbank(H.source_toothbank)
{
    H.sense = '\0';
    H.source_bank = nullptr;
    H.source_toothbank = nullptr;
}

/**
 * The move-assigned-from HyperGraph \p H is left null but valid as if default
 * constructed.
 */
HyperGraph &HyperGraph::operator=(Sep::HyperGraph &&H) noexcept
{
    sense = H.sense;
    H.sense = '\0';

    rhs = H.rhs;

    cliques = std::move(H.cliques);
    teeth = std::move(H.teeth);

    source_bank = H.source_bank;
    H.source_bank = nullptr;

    source_toothbank = H.source_toothbank;
    H.source_toothbank = nullptr;

    return *this;
}

HyperGraph::~HyperGraph()
{
    if (source_bank != nullptr)
        for (Clique::Ptr &ref : cliques)
            source_bank->del_clique(ref);

    if (source_toothbank != nullptr)
        for (Tooth::Ptr &ref : teeth)
            source_toothbank->del_tooth(ref);
}

HyperGraph::Type HyperGraph::cut_type() const
{
    using Type = HyperGraph::Type;

    if (source_bank == nullptr && source_toothbank == nullptr)
        return Type::Non;

    if (!teeth.empty())
        return Type::Domino;

    return (cliques.size() == 1) ? Type::Subtour : Type::Comb;
}

double HyperGraph::get_coeff(int end0, int end1) const
{
    if (end0 == end1)
        throw logic_error("Edge has same endpoints in HyperGraph::get_coeff.");

    if (cut_type() == Type::Non)
        throw logic_error("Tried HyperGraph::get_coeff on Non cut.");

    double result = 0.0;

    if (cut_type() != Type::Domino) {
        const vector<int> &perm = source_bank->ref_perm();

        int end0_ind = perm[end0];
        int end1_ind = perm[end1];

        for (const Clique::Ptr &clq_ref : cliques) {
            bool contains_end0 = clq_ref->contains(end0_ind);
            bool contains_end1 = clq_ref->contains(end1_ind);

            if (contains_end0 != contains_end1)
                result += 1.0;
        }

        return result;
    }

    //else it is a Simple DP
    int pre_result = 0;
    //get handle coeffs
    const vector<int> &handle_perm = source_bank->ref_perm();
    int end0_ind = handle_perm[end0];
    int end1_ind = handle_perm[end1];

    const Clique::Ptr &handle_clq = cliques[0];

    bool contains_end0 = handle_clq->contains(end0_ind);
    bool contains_end1 = handle_clq->contains(end1_ind);

    if (contains_end0 && contains_end1) //in E(H)
        pre_result += 2;
    else if (contains_end0 != contains_end1) // in delta(H)
        pre_result += 1;

    const vector<int> &tooth_perm = source_toothbank->ref_perm();

    end0_ind = tooth_perm[end0];
    end1_ind = tooth_perm[end1];
    contains_end0 = false;
    contains_end1 = false;

    for (const Tooth::Ptr &tooth : teeth) {
        const Clique &root_clq = tooth->set_pair()[0];
        const Clique &bod_clq = tooth->set_pair()[1];

        bool root_end0 = false;
        bool root_end1 = false;

        if ((root_end0 = root_clq.contains(end0_ind)))
            if (bod_clq.contains(end1_ind)) {
                pre_result += 1;
                continue;
            }

        if ((root_end1 = root_clq.contains(end1_ind)))
            if (bod_clq.contains(end0_ind)) {
                pre_result += 1;
                continue;
            }

        if (root_end0 || root_end1)
            continue;

        if (bod_clq.contains(end0_ind) && bod_clq.contains(end1_ind))
            pre_result += 2;
    }

    pre_result /= 2;
    return static_cast<double>(pre_result);
}

ExternalCuts::ExternalCuts(const vector<int> &tour, const vector<int> &perm)
try : node_count(tour.size()), clique_bank(tour, perm), tooth_bank(tour, perm)
{
} catch (const exception &e) {
    cerr << e.what() << "\n";
    throw runtime_error("ExternalCuts constructor failed.");
}

/**
 * @param[in] cc_lpcut the Concorde cut to be added.
 * @param[in] current_tour the tour active when cc_lpcut was found.
 */
void ExternalCuts::add_cut(const lpcut_in &cc_lpcut,
                           const vector<int> &current_tour)
{
    cuts.emplace_back(clique_bank, cc_lpcut, current_tour);
}

/**
 * @param[in] dp_cut the domino parity inequality to be added.
 * @param[in] rhs the righthand-side of the cut.
 * @param[in] current_tour the tour active when dp_cut was found.
 */
void ExternalCuts::add_cut(const dominoparity &dp_cut, const double rhs,
                           const vector<int> &current_tour)
{
    cuts.emplace_back(clique_bank, tooth_bank, dp_cut, rhs, current_tour);
}

/**
 * @param[in] blossom_handle the handle of the blossom inequality;
 * @param[in] tooth_edges a vector of vectors of length two, consisting of
 * the end points of edges which are the blossom teeth.
 */
void ExternalCuts::add_cut(const vector<int> &blossom_handle,
                           const vector<vector<int>> &tooth_edges)
{
    cuts.emplace_back(clique_bank, blossom_handle, tooth_edges);
}

/**
 * Add a branching constraint or Non HyperGraph cut to the list. Maintains
 * indexing that agrees with the Relaxation for bookkeeping and cut pruning
 * purposes.
 */
void ExternalCuts::add_cut() { cuts.emplace_back(); }

/**
 * @param[in] delset the entry `delset[i]` shall be one if the cut
 * `cuts[i + node_count]` is to be deleted, zero otherwise.
 */
void ExternalCuts::del_cuts(const vector<int> &delset)
{
    using CutType = HyperGraph::Type;

    int i = 0;
    for (HyperGraph &H : cuts) {
        CutType Htype = H.cut_type();
        if (delset[i + node_count] == -1) {
            if (Htype == CutType::Comb || Htype == CutType::Domino)
                cut_pool.emplace_back(std::move(H));
            else
                H.rhs = '\0';
        }
        ++i;
    }

    cout << "Cut pool now has size " << cut_pool.size() << "\n";

    cuts.erase(std::remove_if(cuts.begin(), cuts.end(),
                              [](const HyperGraph &H) {
                                  return H.rhs == '\0';
                              }),
               cuts.end());
}


/**
 * @param[in] end0 one end of the edge to be added
 * @param[in] end1 the other end of the edge to be added
 * @param[in,out] cmatind the indices of the rows having nonzero
 * coefficients for the new edge
 * @param[in,out] cmatval the coefficients corresponding to entries of
 * \p cmatind.
 */
void ExternalCuts::get_col(const int end0, const int end1,
                           vector<int> &cmatind, vector<double> &cmatval) const
{
    runtime_error err("Problem in ExternalCuts::get_col");

    if (end0 == end1) {
        cerr << "Edge has same endpoints.\n";
        throw err;
    }

    cmatind.clear();
    cmatval.clear();

    int lp_size = node_count + cuts.size();

    try {
        cmatind.reserve(lp_size);
        cmatval.reserve(lp_size);

        cmatind.push_back(end0);
        cmatind.push_back(end1);

        cmatval.push_back(1.0);
        cmatval.push_back(1.0);

        for (int i = 0; i < cuts.size(); ++i) {
            int index = i + node_count;
            double coeff = cuts[i].get_coeff(end0, end1);

            if (coeff != 0.0) {
                cmatind.push_back(index);
                cmatval.push_back(coeff);
            }
        }
    } CMR_CATCH_PRINT_THROW("Couldn't push back column coeffs/inds", err);
}


}
}
