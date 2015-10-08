
#include <constraints/gecode/handlers/csp_handler.hxx>
#include <constraints/gecode/simple_csp.hxx>
#include <constraints/gecode/helper.hxx>
#include <constraints/gecode/rpg_layer.hxx>
#include <heuristics/relaxed_plan/rpg_data.hxx>
#include <utils/logging.hxx>
#include <utils/printers/gecode.hxx>
#include <languages/fstrips/scopes.hxx>

#include <gecode/driver.hh>

namespace fs0 { namespace gecode {

GecodeActionCSPHandler::GecodeActionCSPHandler(const GroundAction& action)
	:  GecodeCSPHandler(),
	   _action(action)
{
	FDEBUG( "translation", "Gecode Action Handler: Processing Action " << _action.getFullName() << std::endl);
	createCSPVariables();

	registerFormulaConstraints(_action.getConditions());

	for (const auto effect:_action.getEffects()) {
		registerEffectConstraints(effect);
	}

	Helper::postBranchingStrategy(_base_csp);

	// MRJ: in order to be able to clone a CSP, we need to ensure that it is "stable" i.e. propagate all constraints until a fixpoint
	#ifdef DEBUG
	Gecode::SpaceStatus st =
	#endif
	_base_csp.status();

	assert(st != Gecode::SpaceStatus::SS_FAILED); // This should never happen, as it means that the action is (statically) unapplicable.

	index_scopes();
}

void GecodeActionCSPHandler::index_scopes() {
	auto scope = ScopeUtils::computeActionDirectScope(_action);
	std::set<VariableIdx> action_support(scope.begin(), scope.end());
	auto effects = _action.getEffects();

	effect_support_variables.resize(effects.size());
	effect_nested_fluents.resize(effects.size());
	effect_rhs_variables.resize(effects.size());
	effect_lhs_variables.resize(effects.size());
	
	_has_nested_lhs = false;

	for (unsigned i = 0; i < effects.size(); ++i) {
		// Insert first the variables relevant to the particular effect and only then the variables relevant to the
		// action which were not already inserted
		effect_support_variables[i] = ScopeUtils::computeDirectScope(effects[i]);
		std::set<VariableIdx> effect_support(effect_support_variables[i].begin(), effect_support_variables[i].end());

		std::set_difference(
			action_support.begin(), action_support.end(),
			effect_support.begin(), effect_support.end(),
			std::inserter(effect_support_variables[i], effect_support_variables[i].begin()));

		ScopeUtils::TermSet nested;

		// Order matters - we first insert the nested fluents from the particular effect
		ScopeUtils::computeIndirectScope(effects[i], nested);
		effect_nested_fluents[i] = std::vector<fs::FluentHeadedNestedTerm::cptr>(nested.cbegin(), nested.cend());


		// And only then the nested fluents from the action preconditions
		// Actually we don't care that much about repetitions between the two sets of terms, since they are checked anyway when
		// transformed into state variables
		nested.clear();
		for (AtomicFormula::cptr formula:_action.getConditions()) ScopeUtils::computeIndirectScope(formula, nested);
		effect_nested_fluents[i].insert(effect_nested_fluents[i].end(), nested.cbegin(), nested.cend());
		
		effect_rhs_variables[i] = _translator.resolveVariableIndex(effects[i]->rhs(), CSPVariableType::Input);
		
		if (!effects[i]->lhs()->flat()) {
			_has_nested_lhs = true;
		} else {
			effect_lhs_variables[i] = effects[i]->lhs()->interpretVariable({});
		}
	}
	
	if (_has_nested_lhs) {
		effect_lhs_variables = {}; // Just in case
	}
}


SimpleCSP::ptr GecodeActionCSPHandler::instantiate_csp(const GecodeRPGLayer& layer) const {
	SimpleCSP* csp = static_cast<SimpleCSP::ptr>(_base_csp.clone());
	_translator.updateStateVariableDomains(*csp, layer);
	return csp;
}

void GecodeActionCSPHandler::createCSPVariables() {
	IntVarArgs intvars;
	BoolVarArgs boolvars;

	registerFormulaVariables(_action.getConditions(), intvars, boolvars);
	for (const auto effect:_action.getEffects()) {
		registerEffectVariables(effect, intvars, boolvars);
	}

	Helper::update_csp(_base_csp, intvars, boolvars);
}

void GecodeActionCSPHandler::registerEffectVariables(const fs::ActionEffect::cptr effect, Gecode::IntVarArgs& intvars, Gecode::BoolVarArgs& boolvars) {
	// Register first the RHS variables as input variables
	registerTermVariables(effect->rhs(), CSPVariableType::Input, _base_csp, _translator, intvars, boolvars);

	// As for the LHS variable, ATM we only register the subterms (if any) recursively as input CSP variables
	auto nested_lhs = dynamic_cast<fs::FluentHeadedNestedTerm::cptr>(effect->lhs());
	if (nested_lhs) {
		registerTermVariables(nested_lhs->getSubterms(), CSPVariableType::Input, _base_csp, _translator, intvars, boolvars);
	}
}


void GecodeActionCSPHandler::registerEffectConstraints(const fs::ActionEffect::cptr effect) {
	// Register the constraints that correspond to both the LHS and RHS of the effect, excluding the LHS root, if it exists.
	// GecodeCSPHandler::registerTermConstraints(effect->lhs(), CSPVariableType::Output, _base_csp, _translator);
	auto nested_lhs = dynamic_cast<fs::FluentHeadedNestedTerm::cptr>(effect->lhs());
	if (nested_lhs) {
		GecodeCSPHandler::registerTermConstraints(nested_lhs->getSubterms(), CSPVariableType::Input, _base_csp, _translator);
	}
	GecodeCSPHandler::registerTermConstraints(effect->rhs(), CSPVariableType::Input, _base_csp, _translator);

	// And now equate the output variable corresponding to the LHS term with the input variable corresponding to the RHS term
// 	const Gecode::IntVar& lhs_gec_var = _translator.resolveVariable(effect->lhs(), CSPVariableType::Output, _base_csp);
// 	const Gecode::IntVar& rhs_gec_var = _translator.resolveVariable(effect->rhs(), CSPVariableType::Input, _base_csp);
// 	Gecode::rel(_base_csp, lhs_gec_var, Gecode::IRT_EQ, rhs_gec_var);
}


void GecodeActionCSPHandler::compute_support(gecode::SimpleCSP* csp, unsigned actionIdx, RPGData& bookkeeping) const {
	
	FFDEBUG("heuristic", "Computing supports for action " << _action.getFullName());
	DFS<SimpleCSP> engine(csp);
	unsigned num_solutions = 0;
	while (SimpleCSP* solution = engine.next()) {
		FFDEBUG("heuristic", std::endl << "Processing action CSP solution #"<< num_solutions + 1 << ": " << print::csp(_translator, *solution))
		process_solution(solution, actionIdx, bookkeeping);
		++num_solutions;
		delete solution;
	}

	FFDEBUG("heuristic", "Solving the Action CSP completely produced " << num_solutions << " solutions");
}

void GecodeActionCSPHandler::process_solution(SimpleCSP* solution, unsigned actionIdx, RPGData& bookkeeping) const {
	
		const auto& effects = _action.getEffects();

		PartialAssignment solution_assignment;
		if (_has_nested_lhs) solution_assignment = _translator.buildAssignment(*solution);
		
		// We compute, effect by effect, the atom produced by the effect for the given solution, as well as its supports
		for (unsigned i = 0; i < effects.size(); ++i) {
			ActionEffect::cptr effect = effects[i];
			VariableIdx affected = _has_nested_lhs ? effect->lhs()->interpretVariable(solution_assignment) : effect_lhs_variables[i];
			Atom atom(affected, _translator.resolveValueFromIndex(effect_rhs_variables[i], *solution));
			
			auto hint = bookkeeping.getInsertionHint(atom);
			FFDEBUG("heuristic", "Processing effect \"" << *effect << "\" yields " << (hint.first ? "new" : "repeated") << " atom " << atom);


			if (hint.first) { // The value is actually new - let us compute the supports, i.e. the CSP solution values for each variable relevant to the effect.
				Atom::vctrp support = std::make_shared<Atom::vctr>();

				// First extract the supports of the "direct" state variables
				for (VariableIdx variable:effect_support_variables[i]) {
					support->push_back(Atom(variable, _translator.resolveInputStateVariableValue(*solution, variable)));
				}

				// And now of the derived state variables. Note that we keep track dynamically (with the 'insert' set) of the actual variables into which
				// the CSP solution resolves to prevent repetitions
				std::set<VariableIdx> inserted;
				for (auto fluent:effect_nested_fluents[i]) {
					const NestedFluentData& nested_translator = _translator.resolveNestedFluent(fluent);
					VariableIdx variable = nested_translator.resolveStateVariable(*solution);

					if (inserted.find(variable) == inserted.end()) { // Don't push twice to the support the same atom
						ObjectIdx value = _translator.resolveValue(fluent, CSPVariableType::Input, *solution);
						support->push_back(Atom(variable, value));
						inserted.insert(variable);
					}
				}

				// Once the support is computed, we insert the new atom into the RPG data structure
				bookkeeping.add(atom, actionIdx, support, hint.second);
			}
		}
	
	
}


} } // namespaces
