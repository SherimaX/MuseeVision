"""craft_lint.py: verify's step 4, the craft lint.

Reads its rules from Docs/Craft.md itself (the banned words, the function and file limits, the duplicate-block limit),
so the Craft has one home, and checks the project against them:
  - banned words as whole tokens of the identifiers Musee code declares (types, functions, parameters, variables,
    enumerators, macros, namespaces), of file and folder names, and of asset names in strings; Scale is banned only
    inside Light, Matter and the Eye;
  - function length (soft and hard limit) with the Python package lizard when it is installed, otherwise with this
    file's own tokeniser and brace matching;
  - file length (soft and hard limit);
  - duplicated blocks longer than the limit (normalised-line hashing over every code file);
  - no '/Game/' asset path outside Studio.
A soft limit is a warning; everything else is an error. Engine names (what Musee code calls or overrides but doesn't
declare) are exempt.

    python craft_lint.py --craft Craft.md --root <worktree> [--project MuseeVision] [--changed changed.txt] [--report out.json]
Exit code 0 when the craft holds, 1 when it doesn't, 2 when Craft.md can't be read.
"""
import argparse
import hashlib
import json
import os
import re
import sys

CODE = ('.h', '.hpp', '.inl', '.cpp', '.c', '.cs', '.ush', '.usf')
PYTHON = ('.py',)
TEXT = CODE + PYTHON + ('.json', '.ini', '.md', '.txt')
ASSET_NAME = re.compile(r'\b(?:SM|SK|M|MI|MF|MPC|T|L|DA|PM|NS|BP|RT|PP|W)_[A-Za-z0-9_]+')
WORD = re.compile(r'[A-Z]+\d*(?![a-z])|[A-Z]?[a-z]+\d*|\d+')
KEYWORDS = set('''
alignas alignof and asm auto bool break case catch char class const constexpr consteval constinit const_cast continue
co_await co_return co_yield decltype default delete do double dynamic_cast else enum explicit export extern false final
float for friend goto if inline int long mutable namespace new noexcept not nullptr operator or override private
protected public register reinterpret_cast return short signed sizeof static static_assert static_cast struct switch
template this thread_local throw true try typedef typeid typename union unsigned using virtual void volatile while
abstract base sealed readonly internal foreach in is as out ref params get set value var lock checked unchecked
'''.split())
STATEMENT_KEYWORDS = {'return', 'if', 'for', 'foreach', 'while', 'switch', 'case', 'default', 'do', 'else', 'goto',
                      'break', 'continue', 'throw', 'delete', 'co_return', 'try', 'catch', 'using', 'typedef',
                      'friend', 'static_assert', 'operator', 'sizeof', 'template', 'namespace', 'class', 'struct',
                      'enum', 'union', 'lock', 'new', 'yield', 'await'}
SPECIFIERS = {'virtual', 'static', 'inline', 'constexpr', 'consteval', 'const', 'explicit', 'extern', 'mutable',
              'typename', 'public', 'private', 'protected', 'internal', 'override', 'sealed', 'readonly', 'partial',
              'abstract', 'async', 'unsafe', 'volatile', 'unsigned', 'signed', 'FORCEINLINE', 'FORCENOINLINE'}
PARAMETER_PUNCTUATION = {'::', '<', '>', '*', '&', '&&', ',', '[', ']', '...'}
DIRECTIVE = re.compile(r'[ \t]*#(?:[^\n\\]|\\.)*', re.S)


class Report:
    def __init__(self):
        self.errors = []
        self.warnings = []

    def error(self, where, message):
        self.errors.append((where, message))

    def warning(self, where, message):
        self.warnings.append((where, message))


# ---- The rules, read from Craft.md -------------------------------------------------------------------------------

def read_rules(craft_path):
    """The lint's rules as Craft.md states them. Raises ValueError naming any rule whose sentence has changed shape."""
    with open(craft_path, encoding='utf-8') as handle:
        text = ' '.join(handle.read().split())
    rules = {}
    banned = re.search(r'Banned in identifiers and asset names\*\*, as whole tokens: (.+?)\. ', text)
    scoped = re.search(r'`(\w+)` is banned inside ([^.]+)\.', text)
    function = re.search(r'A function [^.]*\. The soft limit is ([\d,]+) lines and the hard limit ([\d,]+)\.', text)
    whole_file = re.search(r'A file [^.]*\. The soft limit is ([\d,]+) lines and the hard limit ([\d,]+)\.', text)
    duplicate = re.search(r'A duplicated block longer than ([\d,]+) lines is a defect', text)
    missing = [name for name, found in (('the banned words', banned), ('the scoped banned word', scoped),
               ('the function limits', function), ('the file limits', whole_file), ('the duplicate limit', duplicate)) if not found]
    if missing:
        raise ValueError(f'Craft.md no longer states {", ".join(missing)} in the sentence this checker reads; '
                         'bring workshop/lib/craft_lint.py up to date with it.')
    rules['banned'] = {word.strip().lower() for word in banned.group(1).split(',') if word.strip()}
    rules['scoped'] = (scoped.group(1).lower(), [part.strip() for part in re.split(r',| and ', scoped.group(2)) if part.strip()])
    number = lambda value: int(value.replace(',', ''))
    rules['function'] = (number(function.group(1)), number(function.group(2)))
    rules['file'] = (number(whole_file.group(1)), number(whole_file.group(2)))
    rules['duplicate'] = number(duplicate.group(1))
    return rules


def words_of(name):
    """The whole tokens of a name: CamelCase words, and the parts between underscores, dashes and dots."""
    tokens = []
    for part in re.split(r'[_\-.\s/\\]+', name):
        tokens.extend(WORD.findall(part))
    return tokens


def in_scale_scope(relative, scope):
    """True when a file lies inside one of the places the scoped word is banned (Light, Matter, the Eye)."""
    parts = relative.split('/')
    for place in scope:
        system = place.replace('the ', '').strip()
        if system == 'Eye':
            if 'Visitor' in parts and any('eye' in (w.lower() for w in words_of(p)) for p in parts):
                return True
        elif len(parts) > 3 and parts[1] == 'Source' and parts[2] == 'Systems' and parts[3] == system:
            return True
    return False


class Names:
    """Checks names against the banned words, remembering where each came from."""

    def __init__(self, rules, report):
        self.banned = rules['banned']
        self.scoped_word, self.scope = rules['scoped']
        self.report = report

    def check(self, name, relative, line, kind):
        lowered = [word.lower() for word in words_of(name)]
        hits = [word for word in lowered if word in self.banned]
        if self.scoped_word in lowered and in_scale_scope(relative, self.scope):
            hits.append(self.scoped_word)
        for hit in dict.fromkeys(hits):
            self.report.error(f'{relative}:{line}', f'the {kind} "{name}" contains the banned word "{hit.capitalize()}".')


# ---- A C-family tokeniser (C++, C#, HLSL) ------------------------------------------------------------------------

TOKEN = re.compile(r'''
    (?P<newline>\n)
  | (?P<space>[ \t\r\f\v]+)
  | (?P<linecomment>//[^\n]*)
  | (?P<blockcomment>/\*.*?\*/)
  | (?P<rawstring>R"(?P<delim>[^(\s]{0,16})\(.*?\)(?P=delim)")
  | (?P<verbatim>\$?@\$?"(?:[^"]|"")*")
  | (?P<string>(?:u8|u|U|L|\$)?"(?:\\.|[^"\\\n])*")
  | (?P<char>(?:u8|u|U|L)?'(?:\\.|[^'\\\n])*')
  | (?P<number>\.?\d(?:[\w.']|[eEpP][+-])*)
  | (?P<identifier>[A-Za-z_]\w*)
  | (?P<operator>::|->|\.\.\.|&&|\|\||<<=|>>=|[-+*/%&|^!=<>]=|\+\+|--|.)
''', re.S | re.X)


class Token:
    __slots__ = ('kind', 'text', 'line')

    def __init__(self, kind, text, line):
        self.kind, self.text, self.line = kind, text, line

    def __repr__(self):
        return f'{self.kind}:{self.text}@{self.line}'


def tokenise(text):
    """Tokens of C-family source, without comments; preprocessor directives become one 'directive' token each."""
    tokens, line, position, line_start = [], 1, 0, True
    while position < len(text):
        if line_start:
            directive = DIRECTIVE.match(text, position)
            if directive:
                body = directive.group(0)
                tokens.append(Token('directive', body.strip(), line))
                line += body.count('\n')
                position += len(body)
                continue
        match = TOKEN.match(text, position)
        kind = match.lastgroup
        value = match.group(0)
        if kind in ('identifier', 'number', 'string', 'rawstring', 'verbatim', 'char', 'operator'):
            tokens.append(Token('string' if kind in ('rawstring', 'verbatim') else kind, value, line))
        line += value.count('\n')
        line_start = kind == 'newline' or (line_start and kind == 'space')
        position = match.end()
    return tokens


def matching(tokens, index, opening, closing):
    """The index of the token closing the bracket opened at tokens[index]."""
    depth = 0
    for position in range(index, len(tokens)):
        text = tokens[position].text
        if text == opening:
            depth += 1
        elif text == closing:
            depth -= 1
            if depth == 0:
                return position
    return len(tokens) - 1


def is_macro(text):
    return re.fullmatch(r'[A-Z][A-Z0-9_]+', text) is not None and text not in SPECIFIERS


# ---- Declarations: what Musee code names -------------------------------------------------------------------------

class Declarations:
    """Walks a token list, collecting the names the file declares and the extent of every function body."""

    def __init__(self, tokens):
        self.tokens = tokens
        self.names = []       # (name, line, kind)
        self.functions = []   # (name, start line, end line)

    def run(self):
        self.scope(0, len(self.tokens), in_function=False)
        return self

    def scope(self, start, end, in_function):
        index = start
        while index < end:
            index = self.statement(index, end, in_function)

    def statement(self, index, end, in_function):
        """Reads one statement or declaration from index; returns the index after it."""
        tokens = self.tokens
        token = tokens[index]
        if token.kind == 'directive':
            define = re.match(r'#\s*define\s+(\w+)', token.text)
            if define:
                self.names.append((define.group(1), token.line, 'macro'))
            return index + 1
        if token.text in (';', '}', ')', ']', ','):
            return index + 1
        if token.text == '{':
            close = matching(tokens, index, '{', '}')
            self.scope(index + 1, close, in_function)
            return close + 1
        if token.text in ('public', 'private', 'protected', 'internal') and index + 1 < end and tokens[index + 1].text == ':':
            return index + 2
        if is_macro(token.text) and index + 1 < end and tokens[index + 1].text == '(':
            return matching(tokens, index + 1, '(', ')') + 1
        if token.text == 'template' and index + 1 < end and tokens[index + 1].text == '<':
            return self.statement(matching(tokens, index + 1, '<', '>') + 1, end, in_function)
        look = index
        while look < end and tokens[look].text in SPECIFIERS:
            look += 1
        if look < end and tokens[look].text in ('class', 'struct', 'union', 'enum', 'interface', 'namespace') and not in_function:
            return self.type_declaration(look, end)
        if token.text == '~' and not in_function and index + 2 < end and tokens[index + 2].text == '(':
            return self.function(tokens[index + 1], index + 2, end, in_function, names_it=False)
        if token.text in ('using', 'typedef'):
            return self.alias(index, end)
        return self.declaration(index, end, in_function)

    def type_declaration(self, index, end):
        tokens = self.tokens
        keyword = tokens[index].text
        position = index + 1
        if position < end and tokens[position].text in ('class', 'struct'):
            position += 1
        names = []
        while position < end and tokens[position].text not in ('{', ';', ':', '(', '='):
            if tokens[position].kind == 'identifier' and not is_macro(tokens[position].text) and tokens[position].text not in ('final', 'sealed'):
                names.append(tokens[position])
            position += 1
        if position < end and tokens[position].text == ':' and keyword != 'namespace':
            while position < end and tokens[position].text not in ('{', ';'):
                position += 1
        if position >= end or tokens[position].text != '{':
            return position + 1
        declared = names if keyword == 'namespace' else names[-1:]
        for name in declared:
            self.names.append((name.text, name.line, 'namespace' if keyword == 'namespace' else 'type'))
        close = matching(tokens, position, '{', '}')
        if keyword == 'enum':
            self.enumerators(position + 1, close)
        else:
            self.scope(position + 1, close, in_function=False)
        return close + 1

    def enumerators(self, start, end):
        expect = True
        index = start
        while index < end:
            token = self.tokens[index]
            if token.text == '(':
                index = matching(self.tokens, index, '(', ')') + 1
                continue
            if expect and token.kind == 'identifier' and not is_macro(token.text):
                self.names.append((token.text, token.line, 'enumerator'))
                expect = False
            elif token.text == ',':
                expect = True
            index += 1

    def alias(self, index, end):
        tokens = self.tokens
        position = index
        while position < end and tokens[position].text != ';':
            position += 1
        if tokens[index].text == 'using' and index + 2 < end and tokens[index + 2].text == '=':
            self.names.append((tokens[index + 1].text, tokens[index + 1].line, 'type alias'))
        elif tokens[index].text == 'typedef' and position - 1 > index and tokens[position - 1].kind == 'identifier':
            self.names.append((tokens[position - 1].text, tokens[position - 1].line, 'type alias'))
        return position + 1

    def type_units(self, index, end):
        """Reads a declaration's type and name; returns (units, name token, index after the name)."""
        tokens = self.tokens
        units, position, name = 0, index, None
        while position < end:
            token = tokens[position]
            if token.text in SPECIFIERS or (is_macro(token.text) and position + 1 < end and tokens[position + 1].text != '('):
                position += 1
                continue
            if token.kind != 'identifier' or token.text in STATEMENT_KEYWORDS:
                break
            units += 1
            name = token
            position += 1
            while position + 1 < end and tokens[position].text == '::':
                if tokens[position + 1].text == '~' and position + 2 < end:
                    position += 1
                if tokens[position + 1].kind != 'identifier':
                    break
                name = tokens[position + 1]
                position += 2
            if position < end and tokens[position].text == '<':
                position = matching(tokens, position, '<', '>') + 1
            while position < end and tokens[position].text in ('*', '&', '&&', '?', 'const'):
                position += 1
        return units, name, position

    def declaration(self, index, end, in_function):
        """A function or variable declaration starting at index, or any other statement, skipped to its end."""
        tokens = self.tokens
        if tokens[index].kind != 'identifier' or tokens[index].text in STATEMENT_KEYWORDS:
            return self.skip_statement(index, end, in_function)
        units, name, position = self.type_units(index, end)
        follow = tokens[position].text if position < end else ';'
        if units == 1 and follow == '(' and not in_function and name.text[:1].isupper():
            return self.function(name, position, end, in_function, names_it=False, start=index)
        if units < 2 or name is None:
            return self.skip_statement(index, end, in_function)
        if follow == '(':
            return self.function(name, position, end, in_function, start=index)
        if follow in ('=', ';', '{', '[', ',', ':'):
            self.names.append((name.text, name.line, 'variable'))
        return self.skip_statement(position, end, in_function)

    def function(self, name, open_paren, end, in_function, names_it=True, start=None):
        """A function's parameters and body. A constructor or destructor (names_it False) names no new identifier."""
        tokens = self.tokens
        close = matching(tokens, open_paren, '(', ')')
        self.parameters(open_paren + 1, close)
        position = close + 1
        inherited = any(t.text == 'override' for t in tokens[open_paren if start is None else start:open_paren])
        while position < end and tokens[position].text not in ('{', ';', '=', '=>'):
            if tokens[position].text == 'override':
                inherited = True
            if tokens[position].text == '(':
                position = matching(tokens, position, '(', ')')
            position += 1
        if names_it and not inherited:
            self.names.append((name.text, name.line, 'function'))
        if position < end and tokens[position].text == '{':
            body_end = matching(tokens, position, '{', '}')
            if not in_function:
                self.functions.append((name.text, name.line, tokens[body_end].line))
            self.scope(position + 1, body_end, in_function=True)
            return body_end + 1
        return self.skip_statement(position, end, in_function)

    def parameters(self, start, end):
        tokens = self.tokens
        piece_start = start
        depth = 0
        for position in range(start, end + 1):
            text = tokens[position].text if position < end else ','
            if text in ('(', '<', '[', '{'):
                depth += 1
            elif text in (')', '>', ']', '}'):
                depth -= 1
            elif text == ',' and depth == 0:
                self.parameter(piece_start, position)
                piece_start = position + 1

    def parameter(self, start, end):
        """One parameter's name. A piece holding an expression is an argument (a variable built with constructor
        arguments reads like a function declaration), and names nothing."""
        tokens = [t for t in self.tokens[start:end]]
        for index, token in enumerate(tokens):
            if token.text == '=':
                tokens = tokens[:index]
                break
        if any(t.kind in ('string', 'char') or (t.kind == 'operator' and t.text not in PARAMETER_PUNCTUATION) for t in tokens):
            return
        identifiers = [t for t in tokens if t.kind == 'identifier' and t.text not in SPECIFIERS and not is_macro(t.text)]
        if len(identifiers) >= 2 and tokens and tokens[-1].kind == 'identifier':
            self.names.append((tokens[-1].text, tokens[-1].line, 'parameter'))

    def skip_statement(self, index, end, in_function):
        """Skips to the end of a statement, reading any braces inside it as a nested scope."""
        tokens = self.tokens
        position = index
        while position < end and tokens[position].text not in (';', '{', '}'):
            if tokens[position].text == '(':
                position = matching(tokens, position, '(', ')')
            position += 1
        if position < end and tokens[position].text == '{':
            close = matching(tokens, position, '{', '}')
            self.scope(position + 1, close, in_function)
            return close + 1
        return position + 1 if position < end and tokens[position].text == ';' else max(position, index + 1)


# ---- Python files (Matter's graph builders) ----------------------------------------------------------------------

def python_declarations(text):
    """Names a Python file declares, and its functions' extents by indentation."""
    names, functions, lines = [], [], text.split('\n')
    open_functions = []
    for number, line in enumerate(lines, 1):
        stripped = line.strip()
        if not stripped or stripped.startswith('#'):
            continue
        indent = len(line) - len(line.lstrip())
        while open_functions and indent <= open_functions[-1][2]:
            name, start, _, last = open_functions.pop()
            functions.append((name, start, last))
        for item in open_functions:
            item[3] = number
        match = re.match(r'\s*(def|class)\s+(\w+)', line)
        if match:
            names.append((match.group(2), number, 'function' if match.group(1) == 'def' else 'type'))
            if match.group(1) == 'def':
                open_functions.append([match.group(2), number, indent, number])
            listed = re.match(r'\s*\((.*)\)', line[match.end():])
            for piece in (listed.group(1).split(',') if listed else []):
                parameter = re.split(r'[:=]', piece)[0].strip().lstrip('*')
                if re.fullmatch(r'[A-Za-z_]\w*', parameter) and parameter not in ('self', 'cls'):
                    names.append((parameter, number, 'parameter'))
            continue
        assigned = re.match(r'\s*([A-Za-z_]\w*)\s*(?::[^=]*)?=(?!=)', line)
        if assigned:
            names.append((assigned.group(1), number, 'variable'))
    for name, start, _, last in open_functions:
        functions.append((name, start, last))
    return names, functions


# ---- The checks ----------------------------------------------------------------------------------------------------

def function_extents(path, text, fallback):
    """(name, first line, last line) of every function in a file: lizard when installed, else the fallback's."""
    try:
        import lizard
    except ImportError:
        return fallback
    analysis = lizard.analyze_file.analyze_source_code(path, text)
    return [(f.name, f.start_line, f.end_line) for f in analysis.function_list]


def check_file(path, relative, rules, names, report, lengths):
    with open(path, encoding='utf-8-sig', errors='replace') as handle:
        text = handle.read()
    line_count = text.count('\n') + (0 if text.endswith('\n') else 1)
    soft, hard = rules['file']
    if line_count > hard:
        report.error(relative, f'the file has {line_count} lines; the hard limit is {hard}. A file holds one idea.')
    elif line_count > soft:
        report.warning(relative, f'the file has {line_count} lines; the soft limit is {soft}.')
    if relative.lower().endswith(PYTHON):
        declared, functions = python_declarations(text)
        strings = [(m.group(0), text.count('\n', 0, m.start()) + 1) for m in re.finditer(r'"[^"\n]*"|\'[^\'\n]*\'', text)]
    else:
        tokens = tokenise(text)
        walk = Declarations(tokens).run()
        declared, functions = walk.names, walk.functions
        strings = [(t.text, t.line) for t in tokens if t.kind == 'string']
    for name, line, kind in declared:
        names.check(name, relative, line, kind)
    soft, hard = rules['function']
    for name, start, finish in function_extents(path, text, functions):
        length = finish - start + 1
        lengths.append(length)
        if length > hard:
            report.error(f'{relative}:{start}', f'the function {name} is {length} lines; the hard limit is {hard}.')
        elif length > soft:
            report.warning(f'{relative}:{start}', f'the function {name} is {length} lines; the soft limit is {soft}.')
    return strings


def check_strings(strings, relative, names, report):
    studio = relative.startswith('MuseeVision/Source/Studio/')
    for value, line in strings:
        if '/Game/' in value and not studio:
            report.error(f'{relative}:{line}', f'the string {value[:80]} names a /Game/ asset path; only Studio names asset paths.')
        for asset in ASSET_NAME.findall(value):
            names.check(asset, relative, line, 'asset name')


def normalised_lines(path):
    """A file's code lines for duplicate detection: trimmed, spaces collapsed, blanks and comment lines dropped."""
    kept = []
    with open(path, encoding='utf-8-sig', errors='replace') as handle:
        for number, line in enumerate(handle, 1):
            stripped = ' '.join(line.split())
            if stripped and not stripped.startswith(('//', '/*', '*', '#')) or stripped.startswith(('#include', '#define', '#if')):
                kept.append((stripped, number))
    return kept


def check_duplicates(files, limit, report):
    """Blocks of more than `limit` normalised lines that appear twice (in one file or two)."""
    window = limit + 1
    seen = {}
    contents = {}
    for relative, path in files:
        lines = normalised_lines(path)
        contents[relative] = lines
        for start in range(0, len(lines) - window + 1):
            digest = hashlib.sha1('\n'.join(text for text, _ in lines[start:start + window]).encode()).hexdigest()
            seen.setdefault(digest, []).append((relative, start))
    pairs = set()
    for places in seen.values():
        for (first, a), (second, b) in zip(places, places[1:]):
            if not (first == second and abs(a - b) < window):
                pairs.add((first, a, second, b))
    for first, a, second, b in sorted(pairs):
        if (first, a - 1, second, b - 1) in pairs:
            continue
        run = 0
        while (first, a + run + 1, second, b + run + 1) in pairs:
            run += 1
        lines = contents[first]
        span = lines[a + run + window - 1][1] - lines[a][1] + 1
        report.error(f'{first}:{lines[a][1]}', f'{span} lines here repeat at {second}:{contents[second][b][1]}; '
                     'a duplicated block is promoted to its owner.')


def project_files(root, project):
    """Every file under the project's Source and Data folders, and the asset files under Content, as (relative, path)."""
    found = []
    for folder in ('Source', 'Data', 'Content'):
        base = os.path.join(root, project, folder)
        for directory, subfolders, files in os.walk(base):
            subfolders[:] = [s for s in subfolders if s not in ('Intermediate', 'Binaries', '__pycache__')]
            for leaf in files:
                path = os.path.join(directory, leaf)
                found.append((os.path.relpath(path, root).replace('\\', '/'), path))
    return sorted(found)


def check_names_of_paths(files, names):
    checked = set()
    for relative, _ in files:
        parts = relative.split('/')
        for depth in range(2, len(parts) + 1):
            piece = '/'.join(parts[:depth])
            if piece in checked:
                continue
            checked.add(piece)
            leaf = parts[depth - 1]
            stem = leaf.split('.')[0] if depth == len(parts) else leaf
            names.check(stem, relative if depth == len(parts) else piece, 1, 'file name' if depth == len(parts) else 'folder name')


def lint(root, project, craft):
    rules = read_rules(craft)
    report = Report()
    names = Names(rules, report)
    files = project_files(root, project)
    check_names_of_paths(files, names)
    lengths = []
    code_files = []
    for relative, path in files:
        lowered = relative.lower()
        if lowered.endswith(CODE + PYTHON):
            code_files.append((relative, path))
            strings = check_file(path, relative, rules, names, report, lengths)
            check_strings(strings, relative, names, report)
        elif lowered.endswith(TEXT):
            with open(path, encoding='utf-8-sig', errors='replace') as handle:
                for number, line in enumerate(handle, 1):
                    if '/Game/' in line and not relative.startswith(f'{project}/Source/Studio/'):
                        report.error(f'{relative}:{number}', 'names a /Game/ asset path; only Studio names asset paths.')
    check_duplicates(code_files, rules['duplicate'], report)
    return report, len(code_files), lengths


def main():
    parser = argparse.ArgumentParser(description='The craft lint, with its rules read from Craft.md.')
    parser.add_argument('--craft', required=True)
    parser.add_argument('--root', required=True, help='the worktree')
    parser.add_argument('--project', default='MuseeVision')
    parser.add_argument('--changed', help='a file listing the paths the order changed, one per line; their findings come first')
    parser.add_argument('--report', help='where to write the findings as JSON')
    arguments = parser.parse_args()
    try:
        report, count, lengths = lint(os.path.abspath(arguments.root), arguments.project, arguments.craft)
    except (OSError, ValueError) as problem:
        print(f'ERROR craft: {problem}')
        return 2
    changed = set()
    if arguments.changed and os.path.exists(arguments.changed):
        with open(arguments.changed, encoding='utf-8') as handle:
            changed = {line.strip() for line in handle if line.strip()}
    mark = lambda where: '*' if where.split(':')[0] in changed else ' '
    ordering = lambda item: (mark(item[0]) != '*', item[0])
    for where, message in sorted(report.errors, key=ordering):
        print(f'ERROR{mark(where)}craft: {where}: {message}')
    for where, message in sorted(report.warnings, key=ordering):
        print(f'WARN {mark(where)}craft: {where}: {message}')
    engine = 'lizard' if _has_lizard() else 'its own brace matching (lizard is not installed)'
    print(f'craft: {len(report.errors)} errors, {len(report.warnings)} warnings over {count} code files and '
          f'{len(lengths)} functions; function lengths measured with {engine}.')
    if arguments.report:
        with open(arguments.report, 'w', encoding='utf-8') as handle:
            json.dump({'errors': [f'{w}: {m}' for w, m in report.errors], 'warnings': [f'{w}: {m}' for w, m in report.warnings]},
                      handle, indent=2)
    return 1 if report.errors else 0


def _has_lizard():
    try:
        import lizard  # noqa: F401
        return True
    except ImportError:
        return False


if __name__ == '__main__':
    sys.exit(main())
