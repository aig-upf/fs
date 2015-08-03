
#include <problem_info.hxx>
#include <languages/fstrips/schemata.hxx>
#include <languages/fstrips/language.hxx>
#include "builtin.hxx"
#include <problem.hxx>
#include <utils/utils.hxx>
#include <state.hxx>
#include <external/repository.hxx>

namespace fs0 { namespace language { namespace fstrips {


void process_subterms(const ObjectIdxVector& binding, const ProblemInfo& info, const std::vector<TermSchema::cptr>& subterms, std::vector<const Term*>& processed, std::vector<ObjectIdx>& constants) {
	assert(processed.empty() && constants.empty());
	for (const TermSchema* unprocessed_subterm:subterms) {
		const Term* processed_subterm = unprocessed_subterm->process(binding, info);
		processed.push_back(processed_subterm);
		
		if (const Constant* constant = dynamic_cast<const Constant*>(processed_subterm)) {
			constants.push_back(constant->getValue());
		}
	}
}
	
std::ostream& TermSchema::print(std::ostream& os) const { return print(os, Problem::getCurrentProblem()->getProblemInfo()); }

std::ostream& TermSchema::print(std::ostream& os, const fs0::ProblemInfo& info) const { 
	os << "<unnamed unprocessed term>";
	return os;
}

Term::cptr NestedTermSchema::process(const ObjectIdxVector& binding, const ProblemInfo& info) const {
	std::vector<const Term*> st;
	std::vector<ObjectIdx> constant_values;
	process_subterms(binding, info, _subterms, st, constant_values);
	
	// We process the 4 different possible cases separately:
	const auto& function = info.getFunctionData(_symbol_id);
	if (function.isStatic() && constant_values.size() == _subterms.size()) { // If all subterms are constants, we can resolve the value of the term schema statically
		for (const auto ptr:st) delete ptr;
		auto value = function.getFunction()(constant_values);
		return new Constant(value);
	}
	else if (function.isStatic() && constant_values.size() != _subterms.size()) { // We have a statically-headed nested term
		return new UserDefinedStaticTerm(_symbol_id, st);
	}
	else if (!function.isStatic() && constant_values.size() == _subterms.size()) { // If all subterms were constant, and the symbol is fluent, we have a state variable
		VariableIdx id = info.getVariableId(_symbol_id, constant_values);
		for (const auto ptr:st) delete ptr;
		return new StateVariable(id);
	}
	else {
		return new FluentHeadedNestedTerm(_symbol_id, st);
	}
}


std::ostream& NestedTermSchema::print(std::ostream& os, const fs0::ProblemInfo& info) const {
	return NestedTerm::printFunction(os, info, _symbol_id, _subterms);
}

Term::cptr BuiltinNestedTermSchema::process(const ObjectIdxVector& binding, const ProblemInfo& info) const {
	std::vector<const Term*> st;
	std::vector<ObjectIdx> constant_values;
	process_subterms(binding, info, _subterms, st, constant_values);
	
	auto processed = BuiltinTermFactory::create(_symbol, st);
	if (constant_values.size() == _subterms.size()) { // If all subterms are constants, we can resolve the value of the term schema statically
		auto value = processed->interpret({});
		delete processed;
		return new Constant(value);
	}
	else return processed;
}


std::ostream& BuiltinNestedTermSchema::print(std::ostream& os, const fs0::ProblemInfo& info) const {
	os << *_subterms[0] << _symbol << *_subterms[1];
	return os;
}

Term::cptr ActionSchemaParameter::process(const ObjectIdxVector& binding, const ProblemInfo& info) const {
	assert(_position < binding.size());
	return new Constant(binding.at(_position));
}

std::ostream& ActionSchemaParameter::print(std::ostream& os, const fs0::ProblemInfo& info) const {
	os << "?" << _position;
	return os;
}

Term::cptr ConstantSchema::process(const ObjectIdxVector& binding, const ProblemInfo& info) const {
	return new Constant(_value);
}

std::ostream& ConstantSchema::print(std::ostream& os, const fs0::ProblemInfo& info) const {
	os << _value;
	return os;
}


AtomicFormulaSchema::cptr AtomicFormulaSchema::create(const std::string& symbol, const std::vector<TermSchema::cptr>& subterms) {
	
	auto it = RelationalFormula::string_to_symbol.find(symbol);
	if (it != RelationalFormula::string_to_symbol.end()) { // We have a relation formula
		if (subterms.size() != 2) std::runtime_error("Only binary relational formulas are accepted");
		return new RelationalFormulaSchema(it->second, subterms);
	}
	else { // We assume the formula semantics is externally defined
		return new ExternalFormulaSchema(symbol, subterms);
	}
}

AtomicFormula::cptr ExternalFormulaSchema::process(const ObjectIdxVector& binding, const ProblemInfo& info) const {
	std::vector<const Term*> st;
	std::vector<ObjectIdx> constant_values;
	process_subterms(binding, info, _subterms, st, constant_values);
	
	auto processed = ExternalComponentRepository::instance().instantiate_formula(symbol, st);
	
	if (constant_values.size() == _subterms.size()) { // We can resolve the value of the formula statically
		auto resolved = processed->interpret({}) ? static_cast<AtomicFormula::cptr>(new TrueFormula) : static_cast<AtomicFormula::cptr>(new FalseFormula);
		delete processed;
		return resolved;
	}
	
	return processed;
}


AtomicFormula::cptr RelationalFormulaSchema::process(const ObjectIdxVector& binding, const ProblemInfo& info) const {
	std::vector<const Term*> st;
	std::vector<ObjectIdx> constant_values;
	process_subterms(binding, info, _subterms, st, constant_values);
	
	auto processed = RelationalFormula::create(symbol, st);
	
	if (constant_values.size() == _subterms.size()) { // We can resolve the value of the formula statically
		auto resolved = processed->interpret({}) ? static_cast<AtomicFormula::cptr>(new TrueFormula) : static_cast<AtomicFormula::cptr>(new FalseFormula);
		delete processed;
		return resolved;
	}
	
	return processed;
}

std::ostream& AtomicFormulaSchema::print(std::ostream& os) const { return print(os, Problem::getCurrentProblem()->getProblemInfo()); }

std::ostream& ExternalFormulaSchema::print(std::ostream& os, const fs0::ProblemInfo& info) const {
	os << symbol << "(";
	for (const auto term:_subterms) os << *term << ", ";
	os << ")";
	return os;
}

std::ostream& RelationalFormulaSchema::print(std::ostream& os, const fs0::ProblemInfo& info) const { 
	os << *_subterms[0] << " " << RelationalFormula::symbol_to_string.at(symbol) << " " << *_subterms[1];
	return os;
}


ActionEffect::cptr ActionEffectSchema::process(const ObjectIdxVector& binding, const ProblemInfo& info) const {
	return new ActionEffect(lhs->process(binding, info), rhs->process(binding, info));
}

std::ostream& ActionEffectSchema::print(std::ostream& os) const { return print(os, Problem::getCurrentProblem()->getProblemInfo()); }

std::ostream& ActionEffectSchema::print(std::ostream& os, const fs0::ProblemInfo& info) const { 
	os << *lhs << " := " << *rhs;
	return os;
}



} } } // namespaces
