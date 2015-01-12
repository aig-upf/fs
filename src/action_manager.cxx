
#include <cassert>
#include <iosfwd>

#include <action_manager.hxx>
#include <core_changeset.hxx>
#include <relaxed_action_set_manager.hxx>
#include <utils/projections.hxx>

namespace aptk { namespace core {


bool ActionManager::checkPlanSuccessful(const Problem& problem, const ActionPlan& plan, const State& s0) {
	auto endState = applyPlan(problem, plan, s0);
	// Check first that the plan is valid (pointer not null) and then that leads to a goal state
	return endState && problem.isGoal(*endState);
}

State::ptr ActionManager::applyPlan(const Problem& problem, const ActionPlan& plan, const State& s0) {
	State::ptr s = std::make_shared<State>(s0);
	for (const auto& idx:plan) {
		s = applyAction(problem.getAction(idx), s);
		if (!s) return s;
	}
	return s;
}	

State::ptr ActionManager::applyAction(const CoreAction::cptr& action, const State::ptr& s0) {
	SimpleActionSetManager manager(*s0, Problem::getCurrentProblem()->getConstraints());
	
	if (!manager.isApplicable(*action)) {
		return State::ptr();
	}
	
	Changeset changeset;
	manager.computeChangeset(*action, changeset);
	return std::make_shared<State>(*s0, changeset); // Copy everything into the new state and apply the changeset
}


//! Returns true iff the given plan is valid AND leads to a goal state, when applied to state s0.
bool ActionManager::checkRelaxedPlanSuccessful(const Problem& problem, const ActionPlan& plan, const State& s0) {
	RelaxedState relaxed(s0);
	if (!applyRelaxedPlan(problem, plan, relaxed)) return false;
	return problem.getConstraintManager()->isGoal(relaxed);
}

//! Applies the given plan in relaxed mode to the given relaxed state.
bool ActionManager::applyRelaxedPlan(const Problem& problem, const ActionPlan& plan, RelaxedState& relaxed) {
	for (const auto& idx:plan) {
		if (!applyRelaxedAction(*(problem.getAction(idx)), relaxed)) return false;
	}
	return true;
}

bool ActionManager::applyRelaxedAction(const CoreAction& action, RelaxedState& s) {
	RelaxedActionSetManager manager(Problem::getCurrentProblem()->getConstraints());
	DomainMap projection = Projections::projectToActionVariables(s, action);
	auto res = manager.isApplicable(action, projection);
	
	if (res.first) { // The action is applicable
		Changeset changeset;
		manager.computeChangeset(action, projection, changeset);
		s.accumulate(changeset);
	}
	return res.first;
}


} } // namespaces
