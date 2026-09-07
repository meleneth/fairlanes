import importlib.util
from pathlib import Path
import unittest

script = Path(__file__).resolve().parents[2] / 'cmake/run_iwyu.py'
spec = importlib.util.spec_from_file_location('iwyu_runner', script)
iwyu = importlib.util.module_from_spec(spec)
spec.loader.exec_module(iwyu)


class IwyuCommand(unittest.TestCase):
    def test_gcc_pch_is_removed_but_normal_forced_includes_remain(self):
        command = iwyu.iwyu_command({'arguments': [
            'g++', '-include', '/build/cmake_pch.hxx',
            '-include', 'config.hpp', '-ftrack-macro-expansion=0', '-c', 'game.cpp'
        ]}, 'iwyu')
        self.assertEqual(command, ['iwyu', '-include', 'config.hpp', '-c', 'game.cpp'])

    def test_clang_pch_is_removed_from_shell_commands(self):
        command = iwyu.iwyu_command({'command':
            'clang++ -include-pch /build/cmake_pch.hxx.pch -c game.cpp'}, 'iwyu')
        self.assertEqual(command, ['iwyu', '-c', 'game.cpp'])
