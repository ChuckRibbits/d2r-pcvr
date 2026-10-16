// D2R PCVR as the D2RLoader mod D2R-PCVR (steamvr/mod_identity.py): the plugin in
// <game>\mods\D2R-PCVR\d2rloader\plugins\, its ReShade and ReShade's files in
// <game>\mods\D2R-PCVR\reshade\, its OpenXR loader beside the plugin. D2RLoader loads a
// mod's plugins only while that mod runs (-mod D2R-PCVR), so another VR mod in its own
// mod folder (D2R VR as mods\D2R-VR) never shares a game with this one. Pure string
// work on folder paths ending in '\', no Windows calls, so it is tested on its own.
#pragma once
#include <cwctype>
#include <string>
#include <vector>

namespace pcvrlayout {

inline std::wstring Lower(std::wstring s) {
    for (auto& c : s) c = (wchar_t)std::towlower(c);
    return s;
}

// "<game>\" from "<game>\d2rloader\plugins\" or "<game>\mods\<mod>\d2rloader\plugins\";
// "" when the folder is neither.
inline std::wstring GameOfPluginDir(const std::wstring& pluginDir) {
    const std::wstring tail = L"d2rloader\\plugins\\";
    const std::wstring low = Lower(pluginDir);
    if (low.size() < tail.size() || low.compare(low.size() - tail.size(), tail.size(), tail) != 0) return L"";
    // The plugins' scope: the game folder, or a mod folder under its mods folder.
    std::wstring scope = pluginDir.substr(0, pluginDir.size() - tail.size());
    const std::wstring ls = Lower(scope);
    if (ls.size() >= 2) {
        const size_t slash = ls.rfind(L'\\', ls.size() - 2);
        if (slash != std::wstring::npos && slash >= 4 && ls.compare(slash - 4, 5, L"mods\\") == 0 &&
            (slash == 4 || ls[slash - 5] == L'\\'))
            return scope.substr(0, slash - 4);
    }
    return scope;
}

// "<game>\mods\<mod>\" when the plugin is a mod's, else "".
inline std::wstring ModRootOfPluginDir(const std::wstring& pluginDir) {
    const std::wstring game = GameOfPluginDir(pluginDir);
    const std::wstring tail = L"d2rloader\\plugins\\";
    if (game.empty()) return L"";
    const std::wstring scope = pluginDir.substr(0, pluginDir.size() - tail.size());
    return scope.size() > game.size() ? scope : L"";
}

// Where ReShade64.dll and ReShade's own files are: the mod's reshade\ when it has its
// ReShade there, else the game folder (an install from before the mod folder).
inline std::wstring ReShadeDir(const std::wstring& gameDir, const std::wstring& modRoot, bool modHasReShade) {
    if (!modRoot.empty() && modHasReShade) return modRoot + L"reshade\\";
    return gameDir;
}

// The folders whose plugins load into the same game as this one: its own, and D2RLoader's
// global plugins folder when this plugin is a mod's (global plugins load with every mod).
// Another mod's folder is not among them.
inline std::vector<std::wstring> SameGamePluginDirs(const std::wstring& pluginDir) {
    std::vector<std::wstring> dirs{pluginDir};
    if (!ModRootOfPluginDir(pluginDir).empty()) dirs.push_back(GameOfPluginDir(pluginDir) + L"d2rloader\\plugins\\");
    return dirs;
}

}  // namespace pcvrlayout
