/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/** @file
 * @brief ABC search nodes.
 * This file contains the structure representing branching subproblems to be
 * examined, and various comparator functions for implementing node selection
 * rules. There are also some I/O utility functions.
 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef CMR_BRANCH_NODE_H
#define CMR_BRANCH_NODE_H

#include "lp_util.hpp"
#include "util.hpp"

#include <array>
#include <iostream>
#include <list>
#include <utility>


namespace CMR {
namespace ABC {

struct BranchNode {
    enum Dir : int {
        Down = 0,
        Up = 1
    };

    enum class Status {
        NeedsCut,
        NeedsBranch,
        NeedsPrice,
        NeedsRecover,
        Pruned,
        Done,
    };

    BranchNode(); //!< Construct a root node.

    /// Construct a child node.
    BranchNode(EndPts ends_, Dir direction_, const BranchNode &parent_,
               double tourlen_, double estimate_);

    BranchNode(BranchNode &&B) noexcept;
    BranchNode &operator=(BranchNode &&B) noexcept;

    /// Alias declaration for returning two split child problems.
    using Split = std::array<BranchNode, 2>;

    EndPts ends; //!< The endpoints of the branch edge.
    Dir direction; //!< Down branch or up branch.

    Status stat; //!< The type of processing required by the node.

    const BranchNode *parent;
    int depth; //!< Search tree depth of this node.

    double tourlen; //!< Estimated best tour length for this node.

    /// A starting basis for if Status is NeedsPrice or NeedsRecover.
    LP::Basis::Ptr price_basis;
    double estimate; //!< The objective value estimate from edge selection.

    /// Is this the root problem.
    bool is_root() const { return parent == nullptr; }

    /// Has the problem been processed.
    bool visited() const
        { return stat == Status::Pruned || stat == Status::Done; }
};

/// Turn a 0-1 value \p entry into a BranchNode::Dir.
BranchNode::Dir dir_from_int(int entry);

/// A concise "label" for the BranchNode \p B.
std::string bnode_brief(const BranchNode &B);

std::ostream &operator<<(std::ostream &os, const BranchNode::Dir &dir);
std::ostream &operator<<(std::ostream &os, const BranchNode &B);

using BranchHistory = std::list<BranchNode>;

/// Alias declaration for EndPts and branching direction.
using EndsDir = std::pair<EndPts, BranchNode::Dir>;


}
}

#endif
