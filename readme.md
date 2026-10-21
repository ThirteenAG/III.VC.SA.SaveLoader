# Save Loader (for GTA III, Vice City, San Andreas)
[DOWNLOAD](https://github.com/ThirteenAG/III.VC.SA.SaveLoader/releases/latest)

Loads the most recent save (or a chosen slot, or a new game) when the game starts. Hold **Shift** while the game starts to go to the main menu instead.

----
## Installation
1. [ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases) is required. If you don't have it, install [Widescreen Fix](https://thirteenag.github.io/wfp#gta), ASI Loader comes with it.
2. Unpack **III.VC.SA.SaveLoader.asi** into the **scripts** folder, or the [modloader](https://github.com/thelink2012/modloader/releases) folder.
3. Start the game. **III.VC.SA.SaveLoader.cfg** is created next to the plugin with the default settings.

FusionFixHub installs the plugin as an extra of the GTA III, Vice City and San Andreas fixes and edits its settings.

----
## Supported games
* Classic GTA III v1.0, v1.1 and Steam executables
* Classic GTA Vice City v1.0, v1.1 and Steam executables
* Classic GTA San Andreas v1.0 US

----
## Settings
Settings are stored next to the plugin in **III.VC.SA.SaveLoader.cfg** and can be changed there or in FusionFixHub. They apply at the next start of the game.

| Setting | Values | Default |
|---|---|---|
| `MAIN.LoadSlot` | -1 Off, 0 the most recent save, 1-8 that slot, 9 a new game | 0 |
| `MAIN.SkipLoadingScreens` | doesn't draw the loading screens (faster loading in San Andreas) | 1 |
| `MAIN.SkipOutro` | Vice City quits without its closing screen | 1 |
| `MAIN.UserFilesFolder` | a folder in the game's folder for saves and the game's settings, e.g. `save`; empty = the game's folder in Documents | empty |

| Preset | `LoadSlot` | `SkipLoadingScreens` | `SkipOutro` |
|---|---|---|---|
| Vanilla | -1 | 0 | 0 |
| Recommended | 0 | 1 | 1 |
| Enhanced | 0 | 1 | 1 |

----
## Building
Initialize the Git submodules, run `premake5.bat`, then build `build/Tools.slnx` (the settings validator, which checks `source/settings` before every build) and `build/III.VC.SA.SaveLoader.slnx` with Visual Studio 2026 or newer, using **Release / Win32**. `data/release.bat` embeds the PDB, signs the ASI when `CODE_SIGNING_PFX` and `CODE_SIGNING_PASSWORD` are set, and packs the zip. GitHub Actions builds every push; run the workflow manually on `master` with `release: true` to publish a release.
