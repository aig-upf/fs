
#include <problem.hxx>
#include <problem_info.hxx>
#include <constraints/gecode/csp_translator.hxx>

namespace fs0 { namespace gecode {
	
	
bool GecodeCSPVariableTranslator::registerCSPVariable(VariableIdx variable, CSPVariableType type, unsigned csp_variable) {
	auto res = _variables.insert(std::make_pair(
		std::make_pair(variable, type),
		csp_variable
	));
	return res.second;
}

const Gecode::IntVar& GecodeCSPVariableTranslator::resolveVariable(const SimpleCSP& csp, VariableIdx variable, CSPVariableType type) const {
	auto it = _variables.find(std::make_pair(variable, type));
	if(it == _variables.end()) {
		throw std::runtime_error("Trying to translate a non-existing CSP variable");
	}
	return csp._X[ it->second ];
}

Gecode::IntVarArgs GecodeCSPVariableTranslator::resolveFunction(const SimpleCSP& csp, unsigned symbol_id, CSPVariableType type) const {
	const ProblemInfo& info = Problem::getCurrentProblem()->getProblemInfo();
	Gecode::IntVarArgs variables;
	for (VariableIdx variable:info.getFunctionData(symbol_id).getStateVariables()) {
		variables << resolveVariable(csp, variable, type);
	}
	return variables;
}

Gecode::IntVarArgs GecodeCSPVariableTranslator::resolveScope(const SimpleCSP& csp, const VariableIdxVector& scope, CSPVariableType type) const {
	Gecode::IntVarArgs variables;
	for (VariableIdx variable:scope) {
		variables << resolveVariable(csp, variable, type);
	}
	return variables;
}

std::ostream& GecodeCSPVariableTranslator::print(std::ostream& os, const SimpleCSP& csp) const {
	os << "CSP with the following variables: " << std::endl;
	const ProblemInfo& info = Problem::getCurrentProblem()->getProblemInfo();
	for (auto it:_variables) {
		std::string var_name = info.getVariableName(it.first.first);
		if (it.first.second == CSPVariableType::Output) {
			var_name = var_name + "'"; // We mark output variables with a "'"
		}
		os << "\t" << var_name << ": " << csp._X[it.second] << std::endl;
	}
	return os;
}

} } // namespaces
