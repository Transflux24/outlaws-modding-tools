"""Mod Organizer 2 Basic Games support for Star Wars Outlaws."""

import os
import winreg
from pathlib import Path

import mobase
from PyQt6.QtCore import QDir, QFileInfo

from ..basic_game import BasicGame
from ..basic_features import BasicModDataChecker, GlobPatterns


class StarWarsOutlawsModDataChecker(BasicModDataChecker):
    """Recognize common Star Wars Outlaws loose-file mod layouts."""

    def __init__(self) -> None:
        super().__init__(
            GlobPatterns(
                valid=[
                    "helix",
                    "plugins",
                    "scripts",
                    "*.asi",
                    "*.dll",
                    "*.ini",
                    "*.jpg",
                    "*.jpeg",
                    "*.md",
                    "*.pdf",
                    "*.png",
                    "*.txt",
                    "*.url",
                ],
                move={
                    "baked": "helix/",
                    "game system data": "helix/",
                    "graph objects": "helix/",
                    "sdf": "helix/",
                },
            )
        )


class StarWarsOutlawsGame(BasicGame):
    Name = "Star Wars Outlaws Support Plugin"
    Author = "SiDeW"
    Version = "1.0.1"

    GameName = "Star Wars Outlaws"
    GameShortName = "starwarsoutlaws"
    GameNexusName = "starwarsoutlaws"
    GameBinary = "Outlaws.exe"
    GameDataPath = ""
    GameDocumentsDirectory = r"%DOCUMENTS%\My Games\Outlaws"
    GameSteamId = 2842040

    UbisoftRegistryKey = (
        r"SOFTWARE\WOW6432Node\Ubisoft\Launcher\Installs\17903"
    )

    def init(self, organizer: mobase.IOrganizer) -> bool:
        if not super().init(organizer):
            return False
        return self._register_feature(StarWarsOutlawsModDataChecker())

    def detectGame(self) -> None:
        """Try the standard Steam lookup, then Ubisoft Connect's registry entry."""
        super().detectGame()
        if self.isInstalled():
            return

        try:
            with winreg.OpenKey(
                winreg.HKEY_LOCAL_MACHINE, self.UbisoftRegistryKey
            ) as key:
                install_dir, _ = winreg.QueryValueEx(key, "InstallDir")
        except OSError:
            return

        candidate = Path(os.path.expandvars(str(install_dir)))
        if self._has_game_binary(candidate):
            self.setGamePath(candidate)

    def looksValid(self, directory: QDir) -> bool:
        return self._has_game_binary(Path(directory.absolutePath()))

    def executables(self):
        game_dir = self.gameDirectory()
        executables = []

        for label, binary in (
            ("Star Wars Outlaws", "Outlaws.exe"),
            ("Star Wars Outlaws Plus", "Outlaws_Plus.exe"),
        ):
            executable = QFileInfo(game_dir, binary)
            if executable.exists():
                executables.append(
                    mobase.ExecutableInfo(label, executable).withWorkingDirectory(
                        game_dir
                    )
                )

        return executables

    @staticmethod
    def _has_game_binary(directory: Path) -> bool:
        return any(
            (directory / binary).is_file()
            for binary in ("Outlaws.exe", "Outlaws_Plus.exe")
        )
