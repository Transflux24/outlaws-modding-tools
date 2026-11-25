# Outlaws save inspection

These two Python scripts help identify a local save by playtime and other metadata. The first scans for recognizable Zstandard frames and writes their concatenated decompressed contents to a scratch directory. The second searches the extracted bytes for a known metadata record and writes one CSV row per matching file.

Use a copy of a save you trust, an empty output directory, and a Zstandard DLL that matches your Python interpreter's architecture. Obtain the DLL from [Zstandard](https://github.com/facebook/zstd/releases) or a build you trust. A Zstandard frame must declare a usable output size for this helper to decode it.

```powershell
python inspect_outlaws_save.py "PATH_TO_ZSTD_DLL" ".\scratch\raw" "PATH_TO_SAVE_COPY"
python summarize_outlaws_saves.py ".\scratch\raw" ".\scratch\save-summary.csv"
```

The CSV includes playtime, autosave flag, quest and location identifiers, game build, and a timestamp. Quest and location values are internal IDs. Unknown metadata layouts or unmatched records yield no CSV row. The scripts inspect copies and produce disposable data for local comparison. Use a new output path to preserve an earlier extraction or CSV. [License](../../LICENSE)
