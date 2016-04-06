
#include <actions/action_id.hxx>
#include <actions/actions.hxx>
#include <actions/grounding.hxx>
#include <problem.hxx>
#include <problem_info.hxx>
#include <limits>
#include <utils/utils.hxx>
#include <utils/printers/binding.hxx>
#include <utils/printers/actions.hxx>
#include <boost/functional/hash.hpp>

namespace fs0 {

const LiftedActionID LiftedActionID::invalid_action_id = LiftedActionID(nullptr, std::vector<ObjectIdx>());


LiftedActionID::LiftedActionID(const PartiallyGroundedAction* action, Binding&& binding)
	: _action(action), _binding(std::move(binding)), _hash(0), _hashed(false)
{
	// The binding of the PartiallyGroundedAction might be incomplete, but the binding of the LiftedActionID must be complete.
	assert(_binding.is_complete());
}

bool LiftedActionID::operator==(const ActionID& rhs) const {
	// Two lifted actions are equal iff they arise from the same action schema and have the same full binding
	auto derived = dynamic_cast<const LiftedActionID*>(&rhs);
	if(!derived) return false;
	if (hash() != rhs.hash()) return false; // For faster non-equality detection
	return _action->getOriginId() == derived->_action->getOriginId() && _binding == derived->_binding;
}

unsigned PlainActionID::id() const { return _action->getId(); }

bool PlainActionID::operator==(const ActionID& rhs) const {
	auto derived = dynamic_cast<const PlainActionID*>(&rhs);
	if(!derived) return false;
	return  id() == derived->id();
}

std::size_t LiftedActionID::hash() const {
	if (!_hashed) {
		_hash = generate_hash();
		_hashed = true;
	}
	return _hash;
}

std::size_t LiftedActionID::generate_hash() const {
	std::size_t hash = 0;
	const std::vector<ObjectIdx>& binding_data = _binding.get_full_binding();
	boost::hash_combine(hash, typeid(*this).hash_code());
	boost::hash_combine(hash, _action->getOriginId());
	boost::hash_combine(hash, boost::hash_range(binding_data.begin(), binding_data.end()));
	return hash;
}

std::size_t PlainActionID::hash() const {
	std::size_t hash = 0;
	boost::hash_combine(hash, typeid(*this).hash_code());
	boost::hash_combine(hash, id());
	return hash;
}

std::ostream& LiftedActionID::print(std::ostream& os) const {
	os << _action;
	return os;
}

std::ostream& PlainActionID::print(std::ostream& os) const {
	os << _action;
	return os;
}

GroundAction* LiftedActionID::generate() const {
	const ProblemInfo& info = Problem::getInfo();
	return ActionGrounder::bind(*_action, _binding, info);
}

} // namespaces
