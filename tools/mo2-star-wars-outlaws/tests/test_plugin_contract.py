"""Contract tests that do not require a Mod Organizer 2 installation."""

import importlib.util
import sys
import tempfile
import types
import unittest
from pathlib import Path
from unittest import mock


PLUGIN_PATH = Path(__file__).parents[1] / "game_starwarsoutlaws.py"


class QDir:
    def __init__(self, path=""):
        self._path = str(path)

    def absolutePath(self):
        return self._path


class QFileInfo:
    def __init__(self, directory, filename):
        self.path = Path(directory.absolutePath()) / filename

    def exists(self):
        return self.path.is_file()


class ExecutableInfo:
    def __init__(self, label, executable):
        self.label = label
        self.executable = executable
        self.working_directory = None

    def withWorkingDirectory(self, directory):
        self.working_directory = directory
        return self


class BasicGame:
    def __init__(self):
        self._game_path = ""

    def init(self, organizer):
        return True

    def _register_feature(self, feature):
        self.feature = feature
        return True

    def detectGame(self):
        return None

    def isInstalled(self):
        return bool(self._game_path)

    def setGamePath(self, path):
        self._game_path = str(path)

    def gameDirectory(self):
        return QDir(self._game_path)


class GlobPatterns:
    def __init__(self, unfold=None, valid=None, delete=None, move=None):
        self.patterns = {
            "unfold": unfold,
            "valid": valid,
            "delete": delete,
            "move": move or {},
        }


class BasicModDataChecker:
    def __init__(self, patterns):
        self.patterns = patterns


def load_plugin_module():
    mobase = types.ModuleType("mobase")
    mobase.IOrganizer = object
    mobase.ExecutableInfo = ExecutableInfo

    pyqt6 = types.ModuleType("PyQt6")
    qtcore = types.ModuleType("PyQt6.QtCore")
    qtcore.QDir = QDir
    qtcore.QFileInfo = QFileInfo

    basic_games = types.ModuleType("basic_games")
    basic_games.__path__ = []
    games = types.ModuleType("basic_games.games")
    games.__path__ = []
    basic_game = types.ModuleType("basic_games.basic_game")
    basic_game.BasicGame = BasicGame
    basic_features = types.ModuleType("basic_games.basic_features")
    basic_features.BasicModDataChecker = BasicModDataChecker
    basic_features.GlobPatterns = GlobPatterns

    modules = {
        "mobase": mobase,
        "PyQt6": pyqt6,
        "PyQt6.QtCore": qtcore,
        "basic_games": basic_games,
        "basic_games.games": games,
        "basic_games.basic_game": basic_game,
        "basic_games.basic_features": basic_features,
    }
    sys.modules.update(modules)

    spec = importlib.util.spec_from_file_location(
        "basic_games.games.game_starwarsoutlaws", PLUGIN_PATH
    )
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class StarWarsOutlawsPluginTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.module = load_plugin_module()

    def test_metadata_and_layout(self):
        game = self.module.StarWarsOutlawsGame()
        self.assertEqual(game.Version, "1.0.1")
        self.assertEqual(game.GameBinary, "Outlaws.exe")
        self.assertEqual(game.GameDataPath, "")
        self.assertEqual(game.GameSteamId, 2842040)
        self.assertEqual(game.GameNexusName, "starwarsoutlaws")

    def test_ubisoft_detection_and_executables(self):
        with tempfile.TemporaryDirectory() as directory:
            expected = Path(directory)
            (expected / "Outlaws.exe").touch()
            (expected / "Outlaws_Plus.exe").touch()
            key = mock.MagicMock()
            key.__enter__.return_value = key
            with mock.patch.object(self.module.winreg, "OpenKey", return_value=key), mock.patch.object(
                self.module.winreg, "QueryValueEx", return_value=(str(expected), 1)
            ):
                game = self.module.StarWarsOutlawsGame()
                game.detectGame()

            self.assertEqual(Path(game._game_path), expected)
            self.assertTrue(game.looksValid(QDir(expected)))

            executables = game.executables()
            self.assertEqual(
                [executable.label for executable in executables],
                ["Star Wars Outlaws", "Star Wars Outlaws Plus"],
            )

    def test_mod_layout_normalization(self):
        checker = self.module.StarWarsOutlawsModDataChecker()
        patterns = checker.patterns.patterns
        self.assertIn("helix", patterns["valid"])
        self.assertEqual(patterns["move"]["baked"], "helix/")
        self.assertEqual(patterns["move"]["graph objects"], "helix/")


if __name__ == "__main__":
    unittest.main()
