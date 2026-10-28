# Borderlands: The Pre-Sequel Save Editor (PS4 / GoldHEN) - beta

In-game save editor plugin for Borderlands: The Pre-Sequel on jailbroken PS4s with GoldHEN, by Toweliee/CLAUDE AI.

> **Beta:** back up your saves before using it (Apollo Save Tool or the PS4's save backup). Please report problems on the [Issues](https://github.com/Towelieevr/Borderlands-Pre-Sequel-Save-Editor-beta/issues) page.

**Game:** Borderlands: The Handsome Collection - The Pre-Sequel, **CUSA01401 v1.10** only.

**Tabs:** Character, Currency, Backpack (item codes by category), SDUs, Style (menu colours, opacity, font and text size).

## Download

Grab the zip from [Releases](https://github.com/Towelieevr/Borderlands-Pre-Sequel-Save-Editor-beta/releases).

If you use the **Toweliee's Save Editors** installer app, choose **Check for updates online** and it will download new versions from this repo by itself.

## Install

1. Copy the `GoldHEN` folder from the zip to `/data/GoldHEN/` on your PS4 over FTP (`tps_editor.prx` goes in `plugins/`; the icon and `tps_codes.txt` go in `/data/GoldHEN/`).
2. Add the editor to `/data/GoldHEN/plugins.ini` as shown in the zip's `plugins.ini.txt`. If `XeMo.prx` is listed under `[CUSA01401]`, put the editor line **after** it.
3. Start the game, load into gameplay, and hold **L3 + R3** to open the editor.

## Building

`source/` has the plugin code. `build.py plugin tps_editor` expects a `tools/` folder next to it with the GoldHEN Plugins SDK and repository and a PS4 toolchain. None of those are included here.


PLEASE NOTE THIS IS STILL A WORK IN PROGRESS ADVICE IS ALWAYS ACCEPTED THANK YOU FOR YOUR PATIENCE.


## License

Free to use, to modify All i ask is you give some credit or dont up to you.
