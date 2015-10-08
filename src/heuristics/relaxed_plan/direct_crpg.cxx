
#include <limits>

#include <heuristics/relaxed_plan/direct_crpg.hxx>
#include <heuristics/relaxed_plan/relaxed_plan_extractor.hxx>
#include <heuristics/relaxed_plan/rpg_data.hxx>
#include <constraints/filtering.hxx>
#include <utils/logging.hxx>
#include <utils/printers/actions.hxx>
#include <relaxed_state.hxx>
#include <constraints/gecode/rpg_layer.hxx>
#include <state_model.hxx>
#include <constraints/direct/direct_rpg_builder.hxx>
#include <constraints/gecode/gecode_rpg_builder.hxx>


namespace fs0 {

DirectCRPG::DirectCRPG(const FS0StateModel& model, std::vector<std::shared_ptr<DirectActionManager>>&& managers, std::shared_ptr<DirectRPGBuilder> builder) :
	_problem(model.getTask()), _managers(managers), _builder(builder)
{
	FDEBUG("heuristic", "Relaxed Plan heuristic initialized with builder: " << std::endl << *_builder);
}


//! The actual evaluation of the heuristic value for any given non-relaxed state s.
long DirectCRPG::evaluate(const State& seed) {
	
	if (ApplicabilityManager::checkFormulaHolds(_problem.getGoalConditions(), seed)) return 0; // The seed state is a goal
	
	
	RelaxedState relaxed(seed);
	RPGData bookkeeping(seed);
	
	FFDEBUG("heuristic", std::endl << "Computing RPG from seed state: " << std::endl << seed << std::endl << "****************************************");
	
	// The main loop - at each iteration we build an additional RPG layer, until no new atoms are achieved (i.e. the rpg is empty),
	// or we get to a goal graph layer.
	while(true) {
		// Apply all the actions to the RPG layer
		for (unsigned idx = 0; idx < _managers.size(); ++idx) {
			const auto& manager = _managers[idx];
			FFDEBUG("heuristic", "Processing ground action #" << idx << ": " << print::action_name(manager->getAction()));
			manager->process(idx, relaxed, bookkeeping);
		}
		
		FFDEBUG("heuristic", "The last layer of the RPG contains " << bookkeeping.getNumNovelAtoms() << " novel atoms." << std::endl << bookkeeping);
		
		// If there is no novel fact in the rpg, we reached a fixpoint, thus there is no solution.
		if (bookkeeping.getNumNovelAtoms() == 0) {
			return -1;
		}
		
		// unsigned prev_number_of_atoms = relaxed.getNumberOfAtoms();
		relaxed.accumulate(bookkeeping.getNovelAtoms());
		bookkeeping.advanceLayer();
/*
 * RETHINK HOW TO FIT THE STATE CONSTRAINTS INTO THE CSP MODEL
 		
		// Prune using state constraints - TODO - Would be nicer if the whole state constraint pruning was refactored into a single line
		FilteringOutput o = _builder.pruneUsingStateConstraints(relaxed);
		FFDEBUG("heuristic", "State Constraint pruning output: " <<  static_cast<std::underlying_type<FilteringOutput>::type>(o));
		if (o == FilteringOutput::Failure) return std::numeric_limits<unsigned>::infinity();
		if (o == FilteringOutput::Pruned && relaxed.getNumberOfAtoms() <= prev_number_of_atoms) return std::numeric_limits<float>::infinity();
*/
		
		FFDEBUG("heuristic", "RPG Layer #" << bookkeeping.getCurrentLayerIdx() << ": " << relaxed);
		
		long h = computeHeuristic(seed, relaxed, bookkeeping);
		if (h > -1) return h;
	}
}

long DirectCRPG::computeHeuristic(const State& seed, const RelaxedState& state, const RPGData& bookkeeping) {
	Atom::vctr causes;
	if (_builder->isGoal(seed, state, causes)) {
		auto extractor = RelaxedPlanExtractorFactory<RPGData>::create(seed, bookkeeping);
		long cost = extractor->computeRelaxedPlanCost(causes);
		delete extractor;
		return cost;
	} else return -1;
}

} // namespaces
