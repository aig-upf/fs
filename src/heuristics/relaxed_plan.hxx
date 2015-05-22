
#pragma once

#include <vector>
#include <algorithm>

#include <state.hxx>
#include <problem.hxx>
#include <heuristics/rpg_data.hxx>


namespace fs0 {

class RPGraph;

template < typename SearchModel >
class RelaxedPlanHeuristic {
public:
	typedef typename SearchModel::ActionType		Action;	
	typedef std::vector< typename Action::IdType >		PrefOpsVec;

	RelaxedPlanHeuristic( const SearchModel& problem );

	virtual ~RelaxedPlanHeuristic() {}
	
	//! The actual evaluation of the heuristic value for any given non-relaxed state s.
	float evaluate(const State& seed);
	
	//! The computation of the heuristic value. Returns -1 if the RPG layer encoded in the relaxed state is not a goal,
	//! otherwise returns h_{FF}.
	//! To be subclassed in other RPG-based heuristics such as h_max
	virtual float computeHeuristic(const State& seed, const RelaxedState& state, const RPGData& rpgData);
	
	//! Proxy to circumvent the unusual virtual method signature
	// @TODO: MRJ: Remove this method, it is now deprecated
	virtual void eval(const State& s, float& h_val) { h_val = evaluate(s); }
	
	//! So far just act as a proxy, we do not compute the preferred operations yet.
	// @TODO: MRJ: Remove this method, it is not deprecated
	virtual void eval( const State& s, float& h_val,  PrefOpsVec& pref_ops ) { eval(s, h_val); }
	
protected:
	const Problem& _problem;
};

} // namespaces
