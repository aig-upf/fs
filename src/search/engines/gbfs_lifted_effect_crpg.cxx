
#include <search/engines/gbfs_lifted_effect_crpg.hxx>
#include <problem.hxx>
#include <state.hxx>
#include <state_model.hxx>
#include <utils/config.hxx>
#include <utils/logging.hxx>
#include <aptk2/search/algorithms/best_first_search.hxx>
#include <heuristics/relaxed_plan/gecode_crpg.hxx>
#include <heuristics/relaxed_plan/lifted_crpg.hxx>
#include <constraints/gecode/gecode_rpg_builder.hxx>
#include <constraints/gecode/handlers/effect_schema_handler.hxx>
#include <asp/asp_rpg.hxx>
#include <actions/applicable_action_set.hxx>

using namespace fs0::gecode;

namespace fs0 { namespace engines {

std::unique_ptr<FS0SearchAlgorithm> GBFSLiftedEffectCRPG::create(const Config& config, const FS0StateModel& model) const {
	FINFO("main", "Using the lifted-effect base RPG constructor");
	const Problem& problem = model.getTask();
	bool novelty = Config::instance().useNoveltyConstraint();
	bool approximate = Config::instance().useApproximateActionResolution();
	
	LiftedCRPG lifted_crpg(problem, problem.getGoalConditions(), problem.getStateConstraints());
	const TupleIndex& tuple_index = problem.get_tuple_index();
	const std::vector<IndexedTupleset>& symbol_tuplesets = lifted_crpg.get_symbol_tuplesets();
	std::vector<std::shared_ptr<EffectSchemaCSPHandler>> managers = EffectSchemaCSPHandler::create(problem.getActionSchemata(), tuple_index, symbol_tuplesets, approximate, novelty);
	lifted_crpg.set_managers(std::move(managers)); // TODO Probably we don't need this to be shared_ptr's anymore
	
	return std::unique_ptr<FS0SearchAlgorithm>(new aptk::StlBestFirstSearch<SearchNode, LiftedCRPG, FS0StateModel>(model, std::move(lifted_crpg)));
}


} } // namespaces
