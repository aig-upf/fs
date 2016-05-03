
#pragma once

#include <typeindex>
#include <typeinfo>

#include <fs_types.hxx>
#include <languages/fstrips/language.hxx>
#include <constraints/direct/constraint.hxx>
#include <constraints/direct/translators/effects.hxx>
#include <constraints/gecode/translators/component_translator.hxx>


namespace fs = fs0::language::fstrips;

namespace fs0 { namespace print { class logical_registry; } }

namespace fs0 {

//! The LogicalComponentRegistry is a singleton object that provides access to a number of classes and methods
//! that know how to translate from and into FSTRIPS logical elements (terms, formulae).
//! At the moment this mainly includes creating terms and formulae from their symbol names and, most relevantly,
//! providing access to Direct/Gecode translator that know how to transform those particular logical elements
//! into operational constraints.
class LogicalComponentRegistry {
public:
	~LogicalComponentRegistry();
	
	//!
	typedef std::function<const fs::AtomicFormula*(const std::vector<const fs::Term*>&)> FormulaCreator;
	
	//!
	typedef std::function<const fs::Term*(const std::vector<const fs::Term*>&)> TermCreator;
	
	//!
	typedef std::function<DirectConstraint*(const fs::AtomicFormula&)> DirectFormulaTranslator;
	
	static LogicalComponentRegistry& instance();
	
	//! Add a formula creator for formulae with the given symbol to the registry
	void addFormulaCreator(const std::string& symbol, const FormulaCreator& creator);
	
	//! Add a formula creator for formulae with the given symbol to the registry
	void addTermCreator(const std::string& symbol, const TermCreator& creator);
	
	//! Add a Direct Formula translator of the given type to the registry
	void add(const std::type_info& type, const DirectFormulaTranslator& translator);
	
	//! Add a Direct effect translator for effects with RHS of the given type
	void add(const std::type_info& type, const EffectTranslator* translator);
	
	//! Add a Gecode Term translator for the given type to the registry
	void add(const std::type_info& type, const gecode::TermTranslator::cptr translator);
	
	//! Add a Gecode Formula translator for the given type to the registry
	void add(const std::type_info& type, const gecode::FormulaTranslator::cptr translator);
	
	const fs::AtomicFormula* instantiate_formula(const std::string symbol, const std::vector<const fs::Term*>& subterms) const;
	
	const fs::Term* instantiate_term(const std::string symbol, const std::vector<const fs::Term*>& subterms) const;
	
	DirectConstraint* instantiate_direct_constraint(const fs::AtomicFormula& formula) const;
	
	const EffectTranslator* getDirectEffectTranslator(const fs::Term& term) const;
	
	gecode::TermTranslator::cptr getGecodeTranslator(const fs::Term& term) const;
	
	gecode::FormulaTranslator::cptr getGecodeTranslator(const fs::AtomicFormula& formula) const;

	friend class print::logical_registry; // Grant access to the corresponding printer class
	
protected:
	LogicalComponentRegistry();
	
	void registerLogicalElementCreators();
	void registerDirectTranslators();
	void registerGecodeTranslators();
	
	std::map<std::string, FormulaCreator> _formula_creators;
	
	std::map<std::string, TermCreator> _term_creators;
	
	typedef std::unordered_map<std::type_index, DirectFormulaTranslator> DirectTranslatorsTable;
	DirectTranslatorsTable _direct_formula_translators;
	
	
	std::unordered_map<std::type_index, const EffectTranslator*> _direct_effect_translators;
	
	std::unordered_map<std::type_index, gecode::TermTranslator::cptr> _gecode_term_translators;
	std::unordered_map<std::type_index, gecode::FormulaTranslator::cptr> _gecode_formula_translators;
};

class UnregisteredGecodeTranslator : public std::runtime_error {
public:
	template <typename T>
	UnregisteredGecodeTranslator(const T& element) : std::runtime_error(message(element)) {}
	
	template <typename T>
	static std::string message(const T& element) {
		std::ostringstream stream;
		stream << "A Gecode translator is required for element \""<< element << "\", but none was registered";
		return stream.str();
	}
};

} // namespaces
