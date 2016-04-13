
#pragma once

#include <search/drivers/registry.hxx>
#include <search/nodes/heuristic_search_node.hxx>
#include <utils/config.hxx>

namespace fs0 { class GroundStateModel;}

namespace fs0 { namespace language { namespace fstrips { class Formula; } }}
namespace fs = fs0::language::fstrips;

namespace fs0 { namespace drivers {

//! An engine creator for the Greedy Best-First Search drivers coupled with our constrained RPG-based heuristics (constrained h_FF, constrained h_max)
//! The choice of the heuristic is done through template instantiation
template <typename GecodeHeuristic, typename DirectHeuristic>
class GBFSConstrainedHeuristicsCreator : public Driver {
protected:
	typedef HeuristicSearchNode<State, GroundAction> SearchNode;
	
public:
	std::unique_ptr<FS0SearchAlgorithm> create(const Config& config, const GroundStateModel& problem) const;
	
protected:
	
	static Config::CSPManagerType decide_csp_type(const Problem& problem);
	
	static Config::CSPManagerType decide_action_manager_type(const std::vector< const GroundAction*>& actions);
	
	static Config::CSPManagerType decide_builder_type(const fs::Formula* goal_formula, const fs::Formula* state_constraints);
};

} } // namespaces
