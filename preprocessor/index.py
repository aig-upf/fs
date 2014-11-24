"""

"""

from extra import IndexDictionary, bool_string


class CompilationIndex(object):
    def __init__(self, task):
        self.task = task
        self.types = {}  # Type name to type ID.
        self.objects = IndexDictionary()  # object name to object ID.
        self.symbols = {}  # symbol name to symbol ID.
        self.variables = {}  # A state variable index.

        self.index_types(self.task)
        self.index_objects(self.task)
        # self.index_symbols(self.task)

    def index_types(self, task):
        self.types['object'] = 0
        self.types['_bool_'] = 1
        for t in task.types:
            if not t.name in ('object', '_bool_'):
                self.types[t.name] = len(self.types)

    def index_objects(self, task):
        self.objects.index(bool_string(False))  # 0
        self.objects.index(bool_string(True))  # 1
        self.objects.index('undefined')
        for o in task.objects:
            self.objects.index(o.name)

    def index_symbols(self, task):
        self.symbols['_eq_'] = 0
        for symbol_name in task.symbols.keys():
            if not symbol_name in self.symbols:
                self.symbols[symbol_name] = len(self.symbols)
