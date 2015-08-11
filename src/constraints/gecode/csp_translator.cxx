
#include <problem.hxx>
#include <problem_info.hxx>
#include <constraints/gecode/csp_translator.hxx>
#include <constraints/gecode/helper.hxx>
#include <relaxed_state.hxx>
#include <utils/logging.hxx>

namespace fs0 { namespace gecode {

UnregisteredStateVariableError::UnregisteredStateVariableError( const char* what_msg )
	: std::runtime_error( what_msg ) {}

UnregisteredStateVariableError::UnregisteredStateVariableError( const std::string& what_msg )
	: std::runtime_error( what_msg ) {}

UnregisteredStateVariableError::~UnregisteredStateVariableError() {

}


bool GecodeCSPVariableTranslator::isRegistered(const fs::Term::cptr term, CSPVariableType type) const {
	TranslationKey key(term, type);
	return _registered.find(key) != _registered.end();
}

bool GecodeCSPVariableTranslator::registerConstant(fs::Constant::cptr constant, SimpleCSP& csp, Gecode::IntVarArgs& variables) {
	TranslationKey key(constant, CSPVariableType::Input);

	auto it = _registered.find(key);
	if (it!= _registered.end()) return false; // The element was already registered

	unsigned id = variables.size();
	int value = constant->getValue();
	variables << Gecode::IntVar(csp, value, value);

	_registered.insert(it, std::make_pair(key, id)); // Constants are always considered as input variables
	return true;
}

bool GecodeCSPVariableTranslator::registerStateVariable(fs::StateVariable::cptr variable, CSPVariableType type, SimpleCSP& csp, Gecode::IntVarArgs& variables) {
	TranslationKey key(variable, type);
	auto it = _registered.find(key);
	if (it!= _registered.end()) return false; // The element was already registered
	variables << Helper::createPlanningVariable(csp, variable->getValue());

	unsigned id = variables.size()-1;
	_registered.insert(it, std::make_pair(key, id));
	FDEBUG( "translation", "Created planning variable (" << variable->getValue() << ") with id=" << id << std::endl );

	// We now cache state variables in different data structures to allow for a more performant subsequent retrieval
	if (type == CSPVariableType::Input) {
		_input_state_variables.insert(std::make_pair(variable->getValue(), id));
	} else if (type == CSPVariableType::Output) {
		_output_state_variables.insert(std::make_pair(variable->getValue(), id));
	}
	return true;
}

bool GecodeCSPVariableTranslator::registerNestedTerm(fs::NestedTerm::cptr nested, CSPVariableType type, SimpleCSP& csp, Gecode::IntVarArgs& variables) {
	TypeIdx domain_type = Problem::getCurrentProblem()->getProblemInfo().getFunctionData(nested->getSymbolId()).getCodomainType();
	return registerNestedTerm(nested, type, domain_type, csp, variables);
}

bool GecodeCSPVariableTranslator::registerNestedTerm(fs::NestedTerm::cptr nested, CSPVariableType type, TypeIdx domain_type, SimpleCSP& csp, Gecode::IntVarArgs& variables) {
	TranslationKey key(nested, type);
	auto it = _registered.find(key);
	if (it!= _registered.end()) return false; // The element was already registered

	variables << Helper::createTemporaryVariable(csp, domain_type);

	_registered.insert(it, std::make_pair(key, variables.size()-1));
	return true;
}

bool GecodeCSPVariableTranslator::registerNestedTerm(fs::NestedTerm::cptr nested, CSPVariableType type, int min, int max, SimpleCSP& csp, Gecode::IntVarArgs& variables) {
	TranslationKey key(nested, type);
	auto it = _registered.find(key);
	if (it!= _registered.end()) return false; // The element was already registered

	variables << Helper::createTemporaryIntVariable(csp, min, max);

	_registered.insert(it, std::make_pair(key, variables.size()-1));
	return true;
}

const Gecode::IntVar& GecodeCSPVariableTranslator::resolveVariable(fs::Term::cptr term, CSPVariableType type, const SimpleCSP& csp) const {
	auto it = _registered.find(TranslationKey(term, type));
	if(it == _registered.end()) {
		throw UnregisteredStateVariableError("Trying to translate a non-existing CSP variable");
	}
	return csp._X[it->second];
}

Gecode::IntVarArgs GecodeCSPVariableTranslator::resolveVariables(const std::vector<fs::Term::cptr>& terms, CSPVariableType type, const SimpleCSP& csp) const {
	Gecode::IntVarArgs variables;
	for (const Term::cptr term:terms) {
		variables << resolveVariable(term, type, csp);
	}
	return variables;
}

std::ostream& GecodeCSPVariableTranslator::print(std::ostream& os, const SimpleCSP& csp) const {
	os << "Gecode CSP with variables: " << std::endl;
	for (auto it:_registered) {
		os << "\t";
		os << *(it.first.getTerm());
		if (it.first.getType() == CSPVariableType::Output) os << "'"; // We simply mark output variables with a "'"
		os << ": " << csp._X[it.second] << std::endl;
	}
	return os;
}

void GecodeCSPVariableTranslator::updateStateVariableDomains(SimpleCSP& csp, const RelaxedState& layer) const {
	// Iterate over all the input state variables and constrain them accodrding to the RPG layer
	for (const auto& it:_input_state_variables) {
		VariableIdx variable = it.first;
		unsigned csp_variable_id = it.second;
		const DomainPtr& domain = layer.getValues(variable);
		Helper::constrainCSPVariable(csp, csp_variable_id, domain);
	}
}

} } // namespaces
