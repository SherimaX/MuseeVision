"""test_checks.py: tests verify's two Python checkers, wiring_check.py and craft_lint.py, on small made-up projects.

Each test builds a throwaway project folder with just the files it needs, runs a checker over it, and checks that
every planted fault is reported and that clean code passes.

    C:\\Users\\anan9\\miniconda3\\python -m unittest discover -s workshop/tests -p "test_*.py" -v
"""
import json
import os
import shutil
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'lib'))
import craft_lint  # noqa: E402
import wiring_check  # noqa: E402

WORKSHOP = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPOSITORY = os.path.dirname(WORKSHOP)
WIRING = os.path.join(REPOSITORY, 'MuseeVision', 'Docs', 'wiring.json')
CRAFT = os.path.join(REPOSITORY, 'MuseeVision', 'Docs', 'Craft.md')


def write(root, relative, text):
    path = os.path.join(root, relative)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w', encoding='utf-8') as handle:
        handle.write(text)
    return path


class Project:
    """A made-up project: module folders with Build.cs files, and the JsonExport a target would give for them."""

    def __init__(self):
        self.root = tempfile.mkdtemp(prefix='musee-check-')
        self.project = os.path.join(self.root, 'MuseeVision')
        self.modules = {}

    def module(self, name, folder, public=(), private=(), base='MuseeModuleRules', rules_text=''):
        directory = os.path.join(self.project, 'Source', folder)
        rules = write(directory, f'{name}.Build.cs',
                      f'public class {name} : {base}\n{{\n\tpublic {name}(ReadOnlyTargetRules Target) : base(Target)\n\t{{\n{rules_text}\t}}\n}}\n')
        automatic = [] if name == 'MuseeFoundation' or base != 'MuseeModuleRules' else ['MuseeFoundation']
        self.modules[name] = {
            'Name': name, 'Directory': directory, 'Rules': rules,
            'PublicDependencyModules': ['Core', 'CoreUObject', 'Engine'] + automatic + list(public),
            'PrivateDependencyModules': list(private), 'PublicIncludePathModules': [], 'DynamicallyLoadedModules': [],
            'CircularlyReferencedModules': [], 'PublicIncludePaths': [os.path.join(self.project, 'Source')],
        }
        return directory

    def export(self, target='MuseeVisionEditor', extra=()):
        engine = {name: {'Name': name, 'Directory': f'C:/Engine/Source/Runtime/{name}', 'PublicDependencyModules': [],
                         'PrivateDependencyModules': []} for name in ('Core', 'CoreUObject', 'Engine', 'HTTP', 'Json', 'UnrealEd', 'OpenXRHMD')}
        engine['UnrealEd']['Directory'] = 'C:/Engine/Source/Editor/UnrealEd'
        modules = dict(engine)
        modules.update(self.modules)
        for name in extra:
            modules.setdefault(name, engine.get(name, {'Name': name, 'Directory': f'C:/Engine/Plugins/{name}'}))
        path = os.path.join(self.root, f'{target}.json')
        with open(path, 'w', encoding='utf-8') as handle:
            json.dump({'Name': target, 'Modules': modules}, handle)
        return path

    def close(self):
        shutil.rmtree(self.root, ignore_errors=True)


class WiringTests(unittest.TestCase):

    def setUp(self):
        self.p = Project()
        self.p.module('MuseeFoundation', 'Systems/Foundation', private=['Json'])
        self.p.module('MuseeTime', 'Systems/Time', private=['HTTP'])
        self.p.module('MuseeMatter', 'Systems/Matter/MuseeMatter', public=['MuseeTime'])
        self.p.module('MuseeForm', 'Systems/Form/MuseeForm', public=['MuseeMatter'])
        self.p.module('MuseeFormMaking', 'Systems/Form/MuseeFormMaking', public=['MuseeForm'])
        self.p.module('MuseeLight', 'Systems/Light', public=['MuseeForm', 'MuseeMatter', 'MuseeTime'])
        self.p.module('MuseeSound', 'Systems/Sound', public=['MuseeForm', 'MuseeMatter', 'MuseeTime'])
        self.p.module('MuseeVisitor', 'Systems/Visitor/MuseeVisitor')

    def tearDown(self):
        self.p.close()

    def errors(self, *exports):
        return wiring_check.check(WIRING, self.p.project, list(exports) or [self.p.export()]).errors

    def test_clean_wiring_passes(self):
        self.assertEqual(self.errors(), [])

    def test_peers_never_see_each_other(self):
        self.p.modules['MuseeLight']['PublicDependencyModules'].append('MuseeSound')
        self.assertTrue(any('MuseeLight depends on MuseeSound' in e for e in self.errors()))

    def test_arrows_point_down(self):
        self.p.modules['MuseeTime']['PrivateDependencyModules'].append('MuseeForm')
        self.assertTrue(any('MuseeTime depends on MuseeForm' in e for e in self.errors()))

    def test_capabilities_have_one_owner(self):
        self.p.modules['MuseeForm']['PrivateDependencyModules'].append('HTTP')
        self.p.modules['MuseeLight']['PrivateDependencyModules'].append('UnrealEd')
        errors = self.errors()
        self.assertTrue(any('MuseeForm uses HTTP; only MuseeTime may use HTTP' in e for e in errors), errors)
        self.assertTrue(any('MuseeLight uses UnrealEd; only MuseeStudio, MuseeMatterMaking may use editor APIs' in e for e in errors), errors)

    def test_unknown_module_is_reported(self):
        self.p.module('MuseeLighting', 'Systems/Lighting')
        self.assertTrue(any('MuseeLighting' in e and 'not in wiring.json' in e for e in self.errors()))

    def test_rooms_see_site_and_the_kit_only(self):
        self.p.module('MuseeSite', 'Systems/Site', public=['MuseeLight', 'MuseeSound', 'MuseeForm', 'MuseeMatter', 'MuseeTime', 'MuseeVisitor'])
        self.p.module('MuseeRotunda', 'Stages/Rooms/Rotunda', public=['MuseeSite', 'MuseeForm', 'MuseeFormMaking'])
        self.p.module('MuseeSalonHall', 'Stages/Rooms/Salon', public=['MuseeSite'])
        errors = self.errors()
        self.assertTrue(any('MuseeRotunda depends on MuseeFormMaking' in e for e in errors), errors)
        self.assertTrue(any('MuseeSalonHall: a stage module is named after its folder (MuseeSalon)' in e for e in errors), errors)
        self.assertFalse(any('MuseeRotunda depends on MuseeForm,' in e for e in errors), errors)

    def test_banned_rules_and_base_class(self):
        self.p.module('MuseeMotion', 'Systems/Motion', public=['MuseeForm'], base='ModuleRules',
                      rules_text='\t\tPublicIncludePaths.Add("x");\n\t\tDynamicallyLoadedModuleNames.Add("MuseeSound");\n')
        errors = self.errors()
        self.assertTrue(any('MuseeMotion.Build.cs uses PublicIncludePaths' in e for e in errors), errors)
        self.assertTrue(any('MuseeMotion.Build.cs uses DynamicallyLoadedModuleNames' in e for e in errors), errors)
        self.assertTrue(any('derives from ModuleRules' in e for e in errors), errors)

    def test_resolved_graph_fields(self):
        self.p.modules['MuseeForm']['CircularlyReferencedModules'] = ['MuseeLight']
        self.p.modules['MuseeForm']['PublicIncludePathModules'] = ['MuseeLight']
        self.p.modules['MuseeForm']['PublicIncludePaths'].append('C:/Elsewhere/Include')
        errors = self.errors()
        self.assertTrue(any('MuseeForm declares circular references' in e for e in errors), errors)
        self.assertTrue(any('MuseeForm reaches headers of MuseeLight' in e for e in errors), errors)
        self.assertTrue(any('MuseeForm adds the include path' in e for e in errors), errors)

    def test_game_targets_never_link_making_modules_or_the_headset(self):
        self.p.module('MuseeVisitorHeadset', 'Systems/Visitor/MuseeVisitorHeadset', public=['MuseeVisitor'], private=['OpenXRHMD'])
        errors = self.errors(self.p.export(), self.p.export('MuseeVision', extra=['OpenXRHMD']))
        self.assertTrue(any('MuseeVision links the editor module MuseeFormMaking' in e for e in errors), errors)
        self.assertTrue(any('MuseeVision links MuseeVisitorHeadset, which belongs only in MuseeVisionVR' in e for e in errors), errors)
        self.assertTrue(any('MuseeVision links OpenXRHMD' in e for e in errors), errors)

    def test_includes(self):
        form = os.path.dirname(self.p.modules['MuseeForm']['Rules'])
        light = os.path.dirname(self.p.modules['MuseeLight']['Rules'])
        sound = os.path.dirname(self.p.modules['MuseeSound']['Rules'])
        write(form, 'Public/FormWall.h', '#pragma once\n')
        write(form, 'Private/FormKernel.h', '#pragma once\n')
        write(sound, 'Public/SoundEmitter.h', '#pragma once\n')
        write(light, 'Private/LightScheme.cpp', '\n'.join([
            '#include "FormWall.h"', '#include "../../Form/MuseeForm/Private/FormKernel.h"',
            '#include "Systems/Form/MuseeForm/Private/FormKernel.h"', '#include "SoundEmitter.h"',
            '// #include "SoundEmitter.h" in a comment is not an include', '#include "CoreMinimal.h"', '']))
        errors = self.errors()
        self.assertTrue(any('LightScheme.cpp:2 includes "../../Form' in e and '".."' in e for e in errors), errors)
        self.assertTrue(any('LightScheme.cpp:3' in e and 'another module\'s Private/' in e for e in errors), errors)
        self.assertTrue(any('LightScheme.cpp:4 includes "SoundEmitter.h" from MuseeSound' in e for e in errors), errors)
        self.assertFalse(any('FormWall.h' in e or 'CoreMinimal' in e or ':5' in e for e in errors), errors)


class CraftTests(unittest.TestCase):

    def setUp(self):
        self.root = tempfile.mkdtemp(prefix='musee-craft-')

    def tearDown(self):
        shutil.rmtree(self.root, ignore_errors=True)

    def lint(self):
        report, _, _ = craft_lint.lint(self.root, 'MuseeVision', CRAFT)
        return [f'{w}: {m}' for w, m in report.errors], [f'{w}: {m}' for w, m in report.warnings]

    def source(self, relative, text):
        write(self.root, f'MuseeVision/Source/{relative}', text)

    def test_clean_code_passes(self):
        self.source('Systems/Form/MuseeForm/Public/FormWall.h', CLEAN)
        self.assertEqual(self.lint(), ([], []))

    def test_banned_words_in_declared_names(self):
        self.source('Systems/Form/MuseeForm/Private/FormHelper.cpp', BANNED)
        errors, _ = self.lint()
        for name in ('FormHelper', 'FFormTempPiece', 'BuildFinal', 'ExtraDepth', 'CopyCount', 'Old', 'FORM_HACK', 'SM_Drum_V2', 'bUseFill'):
            self.assertTrue(any(f'"{name}"' in e for e in errors), f'{name} not reported: {errors}')
        for engine_name in ('SetIntensity', 'NewObject', 'GetBrightness', 'OverrideMaterials'):
            self.assertFalse(any(f'"{engine_name}"' in e for e in errors), f'{engine_name} wrongly reported: {errors}')

    def test_scale_is_banned_only_in_light_matter_and_the_eye(self):
        text = 'namespace Musee::Light\n{\n\tfloat DepthScale(float Scale) { return Scale; }\n}\n'
        self.source('Systems/Light/Private/LightDepth.cpp', text)
        self.source('Systems/Visitor/MuseeVisitor/Private/Eye/VisitorEyeCurve.cpp', text)
        self.source('Systems/Form/MuseeForm/Private/FormDepth.cpp', text)
        errors, _ = self.lint()
        self.assertTrue(any('LightDepth.cpp' in e and '"Scale"' in e for e in errors), errors)
        self.assertTrue(any('VisitorEyeCurve.cpp' in e and '"DepthScale"' in e for e in errors), errors)
        self.assertFalse(any('FormDepth.cpp' in e for e in errors), errors)

    def test_function_and_file_lengths(self):
        body = '\n'.join(f'\tValue += {i};' for i in range(125))
        self.source('Systems/Form/MuseeForm/Private/FormLong.cpp',
                    f'int32 LongOne()\n{{\n\tint32 Value = 0;\n{body}\n\treturn Value;\n}}\n\n'
                    + 'int32 MiddleOne()\n{\n' + '\n'.join('\tCall();' for _ in range(65)) + '\n}\n')
        self.source('Systems/Form/MuseeForm/Private/FormBig.cpp', '\n' * 1005)
        errors, warnings = self.lint()
        self.assertTrue(any('the function LongOne is 130 lines; the hard limit is 120' in e for e in errors), errors)
        self.assertTrue(any('the function MiddleOne is 68 lines; the soft limit is 60' in w for w in warnings), warnings)
        self.assertTrue(any('FormBig.cpp' in e and 'hard limit is 1000' in e for e in errors), errors)

    def test_duplicated_blocks(self):
        block = '\n'.join(f'\tTotal += Weights[{i}] * Samples[{i}];' for i in range(35))
        self.source('Systems/Light/Private/LightA.cpp', f'float SumA()\n{{\n\tfloat Total = 0;\n{block}\n\treturn Total;\n}}\n')
        self.source('Systems/Sound/Private/SoundB.cpp', f'float SumB()\n{{\n\tfloat Total = 0;\n{block}\n\treturn Total;\n}}\n')
        errors, _ = self.lint()
        duplicates = [e for e in errors if 'repeat at' in e]
        self.assertEqual(len(duplicates), 1, errors)
        self.assertIn('LightA.cpp', duplicates[0])

    def test_game_paths_only_in_studio(self):
        self.source('Systems/Time/Private/TimeMoment.cpp', 'const TCHAR* Path = TEXT("/Game/Generated/Time/MPC_Moment");\n')
        self.source('Studio/Private/Make/StudioLevels.cpp', 'const TCHAR* Path = TEXT("/Game/Generated/Site/L_Museum");\n')
        write(self.root, 'MuseeVision/Data/Art/Hang.json', '{"frame": "/Game/Generated/Art/SM_Frame"}\n')
        errors, _ = self.lint()
        self.assertTrue(any('TimeMoment.cpp:1' in e and '/Game/' in e for e in errors), errors)
        self.assertTrue(any('Hang.json:1' in e for e in errors), errors)
        self.assertFalse(any('StudioLevels.cpp' in e for e in errors), errors)

    def test_rules_come_from_craft_md(self):
        rules = craft_lint.read_rules(CRAFT)
        self.assertEqual(rules['function'], (60, 120))
        self.assertEqual(rules['file'], (600, 1000))
        self.assertEqual(rules['duplicate'], 30)
        self.assertIn('manager', rules['banned'])
        self.assertEqual(rules['scoped'][0], 'scale')
        changed = write(self.root, 'Craft.md', '# Craft\nA function is short.\n')
        with self.assertRaises(ValueError):
            craft_lint.read_rules(changed)


CLEAN = '''#pragma once
#include "CoreMinimal.h"

/** A wall as Form draws it: a profile swept along a plan line, in metres. */
struct FFormWall
{
	float HeightMeters = 0.0f;
	float ThicknessMeters = 0.0f;
};

namespace Musee::Form
{
	/** The wall's cross-section area; the plan line's length times this is its volume. */
	inline float SectionArea(const FFormWall& Wall)
	{
		return Wall.HeightMeters * Wall.ThicknessMeters;
	}
}
'''

BANNED = '''#include "FormHelper.h"
#define FORM_HACK 1

struct FFormTempPiece
{
	int32 CopyCount = 0;
	bool bUseFill = false;
};

enum class EFormAge : uint8 { Old, Weathered };

class UFormCaster : public UObject
{
	virtual void OverrideMaterials(int32 Index) override;
	float GetBrightness() const override { return 1.f; }
};

void BuildFinal(float ExtraDepth)
{
	UObject* Piece = NewObject<UObject>();
	Light->SetIntensity(2.f);
	const TCHAR* Mesh = TEXT("SM_Drum_V2");
}
'''

if __name__ == '__main__':
    unittest.main()
