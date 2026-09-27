"""wiring_check.py: verify's step 2, the wiring.

Compares UnrealBuildTool's resolved module graph (the -Mode=JsonExport files of one or more targets) with the
museum's wiring as data (Docs/wiring.json), and reads every Build.cs and #include in the project for what the graph
can't show. It reports, one line each:
  - a Musee module depending on a Musee module its wiring doesn't allow (arrows point down only; peers never meet);
  - an engine capability used by a module that doesn't own it (HTTP is Time's, Json is Foundation's, ...);
  - a module rules file using PublicIncludePaths, PublicIncludePathModuleNames, DynamicallyLoadedModuleNames or
    CircularlyReferencedDependentModules, or not deriving from MuseeModuleRules;
  - an editor module linked into a game target, or a module in a target it is kept out of (Decision D-010);
  - an #include whose path contains '..', reaches into another module's Private/, or takes a header from a module
    the includer's wiring doesn't allow.

    python wiring_check.py --wiring wiring.json --project <worktree>/MuseeVision --export editor.json [--export game.json]
                           [--report wiring.json.out]
Exit code 0 when the wiring holds, 1 when it doesn't, 2 when the inputs can't be read.
"""
import argparse
import fnmatch
import json
import os
import re
import sys

INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.M)
CODE_EXTENSIONS = ('.h', '.hpp', '.inl', '.cpp', '.c')
STAGE_PATTERNS = {
    'room': re.compile(r'^Source/Stages/Rooms/([^/]+)$'),
    'work': re.compile(r'^Source/Stages/Works/([^/]+)/Musee\1$'),
    'workMaking': re.compile(r'^Source/Stages/Works/([^/]+)/Musee\1Making$'),
}


class Findings:
    """The errors and notes found, each one plain sentence."""

    def __init__(self):
        self.errors = []
        self.notes = []

    def error(self, message):
        self.errors.append(message)

    def note(self, message):
        self.notes.append(message)


def slashed(path):
    return os.path.normpath(path).replace('\\', '/')


def is_under(path, folder):
    path, folder = slashed(path).lower(), slashed(folder).lower().rstrip('/') + '/'
    return path.startswith(folder)


def read_json(path):
    with open(path, encoding='utf-8-sig') as handle:
        return json.load(handle)


def strip_comments(text):
    text = re.sub(r'/\*.*?\*/', lambda m: '\n' * m.group(0).count('\n'), text, flags=re.S)
    return re.sub(r'//[^\n]*', '', text)


class Wiring:
    """The wiring as data, with each project module classified as a named module or a stage."""

    def __init__(self, data, project):
        self.data = data
        self.project = project
        self.kit = list(data['kit'])

    def classify(self, name, directory):
        """Returns (spec, stage-kind, stage-name) for a project module, or (None, None, None) when unknown."""
        if name in self.data['modules']:
            return self.data['modules'][name], None, None
        relative = slashed(os.path.relpath(directory, self.project))
        for kind, pattern in STAGE_PATTERNS.items():
            match = pattern.match(relative)
            if match:
                return self.data['stages'][kind], kind, match.group(1)
        return None, None, None

    def allowed(self, name, spec, stage, project_modules):
        """The Musee modules this module may depend on."""
        if spec['musee'] == '*':
            return set(project_modules) - {name}
        allowed = set()
        for entry in spec['musee']:
            if entry == '@kit':
                allowed.update(self.kit)
            elif '<Work>' in entry:
                allowed.add(entry.replace('<Work>', stage or ''))
            else:
                allowed.add(entry)
        if name != self.data['foundation'] and spec.get('rulesBase', self.data['rulesBase']) == self.data['rulesBase']:
            allowed.add(self.data['foundation'])
        return allowed

    def capability_of(self, engine_module, directory):
        for capability in self.data['capabilities']:
            if any(fnmatch.fnmatchcase(engine_module, pattern) for pattern in capability['engine']):
                return capability
            if directory and any(fragment.lower() in slashed(directory).lower() for fragment in capability.get('enginePaths', [])):
                return capability
        return None


def project_modules_of(export, source):
    return {name: module for name, module in export['Modules'].items() if is_under(module['Directory'], source)}


def check_stage_name(name, kind, stage, findings):
    expected = {'room': 'Musee' + (stage or ''), 'work': 'Musee' + (stage or ''), 'workMaking': 'Musee' + (stage or '') + 'Making'}[kind]
    if name != expected:
        findings.error(f'{name}: a stage module is named after its folder ({expected}).')


def check_edges(name, module, wiring, modules, all_modules, findings):
    """The module's Musee dependencies against its wiring, and its engine dependencies against capability owners."""
    spec, kind, stage = wiring.classify(name, module['Directory'])
    if spec is None:
        findings.error(f'{name} ({slashed(os.path.relpath(module["Directory"], wiring.project))}) is not in wiring.json: '
                       'every module is registered there before it is built.')
        return
    if kind:
        check_stage_name(name, kind, stage, findings)
    allowed = wiring.allowed(name, spec, stage, modules)
    depends = list(module.get('PublicDependencyModules', [])) + list(module.get('PrivateDependencyModules', []))
    for dependency in sorted(set(depends)):
        if dependency in modules:
            if dependency not in allowed:
                sees = ', '.join(sorted(allowed)) or 'nothing'
                findings.error(f'{name} depends on {dependency}, which its wiring doesn\'t allow ({name} may see: {sees}).')
            continue
        engine_only = spec.get('engineOnly')
        if engine_only is not None and dependency not in engine_only:
            findings.error(f'{name} depends on {dependency}; it may use only {", ".join(engine_only)}.')
        directory = all_modules.get(dependency, {}).get('Directory', '')
        capability = wiring.capability_of(dependency, directory)
        if capability and name not in capability['owners']:
            if capability['owners']:
                findings.error(f'{name} uses {dependency}; only {", ".join(capability["owners"])} may use {capability["capability"]}.')
            else:
                findings.error(f'{name} uses {dependency}; the museum uses {capability["capability"]} nowhere.')
    unused = sorted(a for a in allowed - set(depends) if a in modules and spec['musee'] != '*')
    if unused and not kind:
        findings.note(f'{name} may see {", ".join(unused)} but doesn\'t use it.')


def check_export_fields(name, module, source, wiring, findings):
    """What the resolved graph shows of the banned rule fields."""
    allowlist = wiring.data.get('bannedRulesAllowlist', {}).get(name, [])
    if module.get('DynamicallyLoadedModules') and 'DynamicallyLoadedModuleNames' not in allowlist:
        findings.error(f'{name} loads modules dynamically ({", ".join(module["DynamicallyLoadedModules"])}); modules find each other by key and signal.')
    if module.get('CircularlyReferencedModules') and 'CircularlyReferencedDependentModules' not in allowlist:
        findings.error(f'{name} declares circular references ({", ".join(module["CircularlyReferencedModules"])}); arrows point down only.')
    extra = sorted(set(module.get('PublicIncludePathModules', [])) - set(module.get('PublicDependencyModules', [])))
    if extra and 'PublicIncludePathModuleNames' not in allowlist:
        findings.error(f'{name} reaches headers of {", ".join(extra)} without depending on them.')
    own = [os.path.join(module['Directory'], leaf) for leaf in ('Public', 'Classes', 'Internal', 'Private')]
    for path in module.get('PublicIncludePaths', []):
        if slashed(path).lower() == slashed(source).lower() or any(slashed(path).lower() == slashed(o).lower() for o in own):
            continue
        if 'PublicIncludePaths' not in allowlist:
            findings.error(f'{name} adds the include path {path}; a module is reached only through its dependencies.')


def check_rules_file(name, module, wiring, findings):
    """The Build.cs text: banned rule fields and the MuseeModuleRules base."""
    rules_path = module.get('Rules', '')
    if not rules_path or not os.path.exists(rules_path):
        return
    with open(rules_path, encoding='utf-8-sig') as handle:
        text = strip_comments(handle.read())
    allowlist = wiring.data.get('bannedRulesAllowlist', {}).get(name, [])
    for field in wiring.data['bannedRules']:
        if re.search(r'\b' + field + r'\b', text) and field not in allowlist:
            findings.error(f'{os.path.basename(rules_path)} uses {field}, which no Musee module rules may use.')
    spec, _, _ = wiring.classify(name, module['Directory'])
    expected = (spec or {}).get('rulesBase', wiring.data['rulesBase'])
    match = re.search(r'\bclass\s+' + re.escape(name) + r'\s*:\s*(\w+)', text)
    if match and match.group(1) != expected:
        findings.error(f'{os.path.basename(rules_path)}: {name} derives from {match.group(1)}; every Musee module derives from {expected}.')


def check_targets(export, modules, wiring, findings):
    """Editor modules never link into a game; modules kept to certain targets stay there (D-010)."""
    target = export['Name']
    rules = wiring.data['targets'].get(target)
    if rules is None:
        findings.error(f'The target {target} is not in wiring.json.')
        return
    if rules['kind'] != 'game':
        return
    for name, module in modules.items():
        spec, _, _ = wiring.classify(name, module['Directory'])
        if spec and spec.get('phase') == 'editor':
            findings.error(f'The game target {target} links the editor module {name}; making modules never reach a game.')
        if spec and 'targets' in spec and target not in spec['targets']:
            findings.error(f'The game target {target} links {name}, which belongs only in {", ".join(spec["targets"])}.')
    for pattern in rules.get('never', []):
        for name in export['Modules']:
            if fnmatch.fnmatchcase(name, pattern):
                findings.error(f'The game target {target} links {name}. {rules.get("why", "")}'.strip())


class IncludeScan:
    """Every #include in the project's C++ checked against module boundaries and the wiring."""

    def __init__(self, modules, source, wiring):
        self.modules = modules
        self.source = source
        self.wiring = wiring
        self.directories = sorted(((slashed(m['Directory']), n) for n, m in modules.items()), key=lambda d: -len(d[0]))
        self.public = {}
        for name, module in modules.items():
            root = os.path.join(module['Directory'], 'Public')
            for folder, _, files in os.walk(root):
                for leaf in files:
                    relative = slashed(os.path.relpath(os.path.join(folder, leaf), root))
                    self.public.setdefault(relative, set()).add(name)

    def owner_of(self, path):
        path = slashed(path).lower()
        for directory, name in self.directories:
            if path.startswith(directory.lower() + '/'):
                return name
        return None

    def run(self, findings):
        for name, module in self.modules.items():
            spec, _, stage = self.wiring.classify(name, module['Directory'])
            allowed = self.wiring.allowed(name, spec, stage, self.modules) if spec else set()
            for folder, _, files in os.walk(module['Directory']):
                for leaf in files:
                    path = os.path.join(folder, leaf)
                    if leaf.lower().endswith(CODE_EXTENSIONS) and self.owner_of(path) == name:
                        self.check_file(path, name, allowed, findings)

    def check_file(self, path, name, allowed, findings):
        with open(path, encoding='utf-8-sig', errors='replace') as handle:
            text = strip_comments(handle.read())
        where = slashed(os.path.relpath(path, os.path.dirname(self.source)))
        for match in INCLUDE.finditer(text):
            included = slashed(match.group(1)) if match.group(1) else ''
            line = text.count('\n', 0, match.start()) + 1
            if '..' in match.group(1).replace('\\', '/').split('/'):
                findings.error(f'{where}:{line} includes "{match.group(1)}"; no include path contains "..".')
                continue
            self.check_target(path, line, where, included, name, allowed, findings)

    def check_target(self, path, line, where, included, name, allowed, findings):
        candidates = [os.path.join(os.path.dirname(path), included), os.path.join(self.source, included)]
        resolved = [c for c in candidates if os.path.isfile(c)]
        if '/private/' in '/' + included.lower():
            owners = {self.owner_of(c) for c in resolved}
            if not resolved or owners != {name}:
                findings.error(f'{where}:{line} includes "{included}" from another module\'s Private/; Private/ is the inside.')
            return
        owners = {self.owner_of(c) for c in resolved if self.owner_of(c)} or self.public.get(included, set())
        if not owners or name in owners:
            return
        if not owners & allowed:
            findings.error(f'{where}:{line} includes "{included}" from {", ".join(sorted(owners))}, which {name}\'s wiring doesn\'t allow.')


def check(wiring_path, project, export_paths):
    findings = Findings()
    wiring = Wiring(read_json(wiring_path), project)
    source = os.path.join(project, 'Source')
    exports = [read_json(path) for path in export_paths]
    for index, export in enumerate(exports):
        modules = project_modules_of(export, source)
        for name, module in sorted(modules.items()):
            check_edges(name, module, wiring, modules, export['Modules'], findings)
            check_export_fields(name, module, source, wiring, findings)
            if index == 0:
                check_rules_file(name, module, wiring, findings)
        check_targets(export, modules, wiring, findings)
    if exports:
        IncludeScan(project_modules_of(exports[0], source), source, wiring).run(findings)
    findings.errors = list(dict.fromkeys(findings.errors))
    findings.notes = list(dict.fromkeys(findings.notes))
    return findings


def main():
    parser = argparse.ArgumentParser(description='Compare the resolved module graph with Docs/wiring.json.')
    parser.add_argument('--wiring', required=True)
    parser.add_argument('--project', required=True, help='the project folder, <worktree>/MuseeVision')
    parser.add_argument('--export', action='append', required=True, help='a -Mode=JsonExport file; the editor target first')
    parser.add_argument('--report', help='where to write the findings as JSON')
    arguments = parser.parse_args()
    try:
        findings = check(arguments.wiring, os.path.abspath(arguments.project), arguments.export)
    except (OSError, ValueError, KeyError) as problem:
        print(f'ERROR wiring: could not read the inputs: {problem}')
        return 2
    for message in findings.errors:
        print(f'ERROR wiring: {message}')
    for message in findings.notes:
        print(f'NOTE  wiring: {message}')
    print(f'wiring: {len(findings.errors)} errors, {len(findings.notes)} notes.')
    if arguments.report:
        with open(arguments.report, 'w', encoding='utf-8') as handle:
            json.dump({'errors': findings.errors, 'notes': findings.notes}, handle, indent=2)
    return 1 if findings.errors else 0


if __name__ == '__main__':
    sys.exit(main())
