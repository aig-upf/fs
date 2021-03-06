"""
    A module to deal with the instantiation of state variables.
"""

import itertools

from base import Variable
from util import is_external, IndexDictionary


def create_all_possible_state_variables(symbols, static_symbols, type_map):
    """ Creates an index with all possible state variables """
    variables = IndexDictionary()

    for symbol in symbols.values():
        name = symbol.name
        if is_external(name) or name in static_symbols:  # The symbol won't yield any state variable
            continue
        instantiations = [type_map[t] for t in symbol.arguments]
        for instantiation in itertools.product(*instantiations):
            variables.add(Variable(symbol.name, instantiation))
    return variables
