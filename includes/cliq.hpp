#ifndef CMR_CLIQ_H
#define CMR_CLIQ_H

#include "tooth.hpp"
#include "util.hpp"

#include <array>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

extern "C" {
#include <concorde/INCLUDE/tsp.h>
}

namespace CMR {
namespace Sep {

/** Class for storing segment lists representing edges of a hypergraph. 
 * A Clique stores a subset of vertices as a list of CMR::Segment objects,
 * where the start and endpoints indicate a range of nodes from a tour. 
 * Thus, a Clique is meaningless without a tour from which to be 
 * derefrenced. 
 */
class Clique {
public:
    Clique() = default;

    /** Construct a Clique from a Concorde clique. 
     * If \p cc_cliq is the clique and \p current_tour was the resident
     * best tour when the clique was found, construct a Clique where the
     * nodes in \p cc_cliq are represented as indices from \p saved_tour with
     * corresponding permutation vector \p saved_perm.
     */
    Clique(const CCtsp_lpclique &cc_cliq,
           const std::vector<int> &saved_tour,
           const std::vector<int> &saved_perm,
           const std::vector<int> & current_tour);

    /** Construct a Clique from start and end indices.
     * If \p current_tour is the resident best tour, constructs the Clique
     * corresponding to the nodes
     * `current_tour[start]` up to `current_tour[end]`,
     * as indices from \p saved_tour.
     */
    Clique(int start, int end,
           const std::vector<int> &saved_tour,
           const std::vector<int> &saved_perm,
           const std::vector<int> & current_tour);

    /** Construct a Clique from a list of literal nodes.
     * The vector \p nodes shall be a list of nodes in the graph relative
     * to some absolute order rather than one dependent on a given tour. 
     * They will be stored in a list of CMR::Segment objects using indices
     * obtained from \p perm, hence implicitly represented in terms of the 
     * tour corresponding to \p perm.
     */
    Clique(std::vector<int> &nodes,
           const std::vector<int> &perm);


    using Ptr = std::shared_ptr<Clique>; /**< shared_ptr alias declaration. */

    /** How many segments are used to represent the Clique. */
    int seg_count() const { return seglist.size(); }

    /** A constant reference to the list of segments in the Clique. */
    const std::vector<CMR::Segment> &seg_list() const { return seglist; }

    /** A list of literal nodes represented by the Clique.
     * Given a Clique, if \p saved_tour was the active tour when the Clique
     * was constructed, this method returns a vector of the literal nodes
     * obtained by dereferencing \p saved_tour for the ranges specified
     * in the segment list. 
     */
    std::vector<int> node_list(const std::vector<int> &saved_tour) const;
    
    /** Equality operator. */
    bool operator==(const Clique &rhs) const { return seglist == rhs.seglist; }
    
private:
    std::vector<CMR::Segment> seglist;
};

/** Vertex set structure used in tooth inequalities for domino parity cuts.
 * This class holds a tooth in the sense used by Fleischer et al. (2006) 
 * meaning two disjoint, nonempty vertex subsets whose union is not the vertex
 * set of the graph. A tooth inequality is obtained by summing the SECs on the
 * two sets. 
 * @remark This underlying data in this class is sufficiently general to hold
 * general teeth (i.e., for general domino parity inequalities), but for now
 * there is only a constructor to implement simple teeth for simple domino 
 * parity inequalities, i.e., teeth where one of the sets is a singleton. 
 */
class Tooth {
public:
    Tooth() = default;

    /** Construct a Tooth from a SimpleTooth.
     * @param saved_tour the dereferencing tour for this Tooth
     * @param saved_perm the dereferencing perm for this Tooth
     * @param current_tour the active tour when \p T was found.
     */
    Tooth(const SimpleTooth &T,
          const std::vector<int> &saved_tour,
          const std::vector<int> &saved_perm,
          const std::vector<int> &current_tour);

    /** Reference to defining set pair. */
    const std::array<Clique, 2> &set_pair() const { return sets; }

    using Ptr = std::shared_ptr<Tooth>; /** Pointer alias declaration. */

    /** Equality operator. */
    bool operator==(const Tooth &rhs) const { return sets == rhs.sets; }

private:
    std::array<Clique, 2> sets;
};


}
}

namespace std {

/** Partial specialization of std::hash taken from CCtsp_hashclique. */
template<>
struct hash<CMR::Sep::Clique> {
    /** Call operator for hashing a Clique. */
    size_t operator()(const CMR::Sep::Clique &clq) const
        {
            size_t val = 0;
        
            for (const CMR::Segment &seg : clq.seg_list())
                val = (val * 65537) + (seg.start * 4099) + seg.end;

            return val;
        }
};

/** Partial specialization of std::hash from CCtsp_hashdomino. */
template<>
struct hash<CMR::Sep::Tooth> {
    /** Call operator for hashing a Tooth. */
    size_t operator()(const CMR::Sep::Tooth &T) const
        {
            size_t val = 0;

            for (const CMR::Sep::Clique &clq : T.set_pair())
                for (const CMR::Segment &seg : clq.seg_list())
                    val = (val * 65537) + (seg.start * 4099) + seg.end;

            return val;
        }
};

}

namespace CMR {
namespace Sep {

/** Storage of a repository of Cliques, for use in building a HyperGraph. 
 * This class is responsible for dispensing and deleting references to Clique
 * objects, for use in the edges of a HyperGraph. The interface allows
 * a Clique to be passed to the CliqueBank. A pointer to the Clique will be
 * added to the bank if one does not exist already, or else the use count
 * of the pointer will be incremented. This pointer is then returned for other
 * use.
 *The Cliques contained
 * therein are meaningless without reference to a fixed tour and perm vector,
 * which shall be used to construct Clique objects and turn them back into
 * lists of nodes or sparse cut rows.
 */
class CliqueBank {
public:
    /** Construct a CliqueBank to be dereferenced by \p tour and \p perm. */
    CliqueBank(const std::vector<int> &tour, const std::vector<int> &perm);

    /** Add a Clique to the bank, and get a reference to it. 
     * This function returns a reference counted pointer to a Clique stored
     * in the bank.
     */
    Sep::Clique::Ptr add_clique(const Sep::Clique &clq);

    /** Construct a Clique in place, add it, and get a reference to it.
     * Same as the overload taking a Clique, but will construct a Clique
     * from the CCtsp_lpclique \p cc_cliq, with \p tour as the tour active
     * when \p cc_cliq was obtained.
     */
    Sep::Clique::Ptr add_clique(const CCtsp_lpclique &cc_cliq,
                                const std::vector<int> &tour);

    /** Construct/add/get a reference to the Clique from endpoints. */
    Sep::Clique::Ptr add_clique(int start, int end,
                                const std::vector<int> &tour);

    /** Construct/add/get a reference to a Clique from a node list.
     * The vector \p nodes shall be a list of nodes to be included in the
     * Clique. 
     * @warning The elements of \p nodes are sorted by this function, but
     * unchanged otherwise.
     */
    Sep::Clique::Ptr add_clique(std::vector<int> &nodes);

    /** Decrement the reference count of a Clique, possibly removing it.
     * The Clique pointed to by \p clq_ptr will be nullified, thereby
     * decrementing the reference count of every other reference to it. 
     * If its reference count in the CliqueBank drops to one, it will be
     * erased, decreasing the size of the CliqueBank.
     */
    void del_clique(Sep::Clique::Ptr &clq_ptr);


    /** The number of unique Cliques in the bank. */
    int size() const { return bank.size(); }

    /** Alias declaration for Clique hash table. */
    using CliqueHash = std::unordered_map<Sep::Clique,
                                          Sep::Clique::Ptr>;


    using Itr = CliqueHash::iterator; /**< Iterator alias. */
    using ConstItr = CliqueHash::const_iterator; /**< Const iter alias. */


    Itr begin() { return bank.begin(); } /**< Begin iterator. */
    ConstItr begin() const { return bank.begin(); } /**< Const begin iter. */

    
    Itr end() { return bank.end(); } /**< Past the end iterator. */    
    ConstItr end() const { return bank.end(); } /**< Const past end iter. */
    
private:
    const std::vector<int> saved_tour;
    const std::vector<int> saved_perm;
    CliqueHash bank;
};


class ToothBank {
public:
    ToothBank(const std::vector<int> &tour, const std::vector<int> &perm);

    Tooth::Ptr add_tooth(const SimpleTooth &T,
                         const std::vector<int> &tour);

    void del_tooth(Tooth::Ptr &T_ptr);

    int size() const { return bank.size(); }

    using ToothHash = std::unordered_map<Tooth, Tooth::Ptr>;

    using Itr = ToothHash::iterator;
    using ConstItr = ToothHash::const_iterator;

    Itr begin() { return bank.begin(); }
    ConstItr begin() const { return bank.begin(); }

    Itr end() { return bank.end(); }
    ConstItr end() const { return bank.end(); }

private:
    const std::vector<int> saved_tour;
    const std::vector<int> saved_perm;

    ToothHash bank;
};

}
}

#endif
