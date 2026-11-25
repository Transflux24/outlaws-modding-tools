# Testing

Run the portable contract tests from the repository root:

```powershell
python -m unittest discover -s tests -v
```

The tests provide lightweight substitutes for MO2 and PyQt interfaces, create temporary fake game executables, and mock Ubisoft registry access. They do not require the game or MO2 to be installed.

Runtime verification should still cover:

1. Detection or manual selection of the game directory.
2. Launch with Ubisoft Connect already open.
3. A simple loose `helix` mod through MO2 virtualization.
4. A USM plugin payload under `plugins` while the proxy DLL remains installed physically beside the executable.
5. Confirmation that saves and game configuration remain shared as intended.

