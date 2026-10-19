#pragma once

#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/render/Texture.hpp>
#include <hyprland/src/config/values/types/BoolValue.hpp>
#include <hyprland/src/config/values/types/IntValue.hpp>
#include <hyprland/src/config/values/types/StringValue.hpp>
#include <hyprland/src/config/values/types/ColorValue.hpp>

inline HANDLE PHANDLE = nullptr;

class CHudDeco;

struct SGlobalState {
    std::vector<WP<CHudDeco>> decos;

    struct {
        SP<Config::Values::CBoolValue>   enabled, ticks, showClose, leds;
        SP<Config::Values::CIntValue>    barHeight, textSize, bracketLen, bracketThick;
        SP<Config::Values::CColorValue>  panel, panelInactive, line, accent, dim, text;
        SP<Config::Values::CStringValue> font;
    } config;
};

inline UP<SGlobalState> g_pGlobalState;
