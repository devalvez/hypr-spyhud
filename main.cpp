#define WLR_USE_UNSTABLE

#include <unistd.h>

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/view/window/Window.hpp>
#include <hyprland/src/desktop/view/window/WindowPresentation.hpp>
#include <hyprland/src/desktop/state/WindowState.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include <hyprland/src/state/MonitorState.hpp>

#include <algorithm>

#include "HudDeco.hpp"
#include "globals.hpp"

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

static void onNewWindow(PHLWINDOW window) {
    if (std::ranges::any_of(window->presentation().decorations(), [](const auto& d) { return d->getDisplayName() == "SpyHud"; }))
        return;

    auto deco = Hyprutils::Memory::makeShared<CHudDeco>(window);
    g_pGlobalState->decos.emplace_back(deco);
    deco->m_self = deco;
    HyprlandAPI::addWindowDecoration(PHANDLE, window, std::move(deco));
}

static void onConfigReloaded() {
    for (auto& d : g_pGlobalState->decos) {
        if (d)
            d->onConfigReloaded();
    }
}

static void onFocusChange() {
    for (auto& d : g_pGlobalState->decos) {
        if (d)
            d->onFocusChange();
    }
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    const std::string HASH        = __hyprland_api_get_hash();
    const std::string CLIENT_HASH = __hyprland_api_get_client_hash();

    if (HASH != CLIENT_HASH) {
        HyprlandAPI::addNotification(PHANDLE, "[spyhud] Failure in initialization: Version mismatch (headers ver is not equal to running hyprland ver)",
                                     CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("[spyhud] Version mismatch");
    }

    g_pGlobalState = makeUnique<SGlobalState>();

    static auto P  = Event::bus()->m_events.window.open.listen([&](PHLWINDOW w) { onNewWindow(w); });
    static auto P2 = Event::bus()->m_events.config.reloaded.listen([&] { onConfigReloaded(); });
    static auto P3 = Event::bus()->m_events.window.active.listen([&](PHLWINDOW, Desktop::eFocusReason) { onFocusChange(); });

    auto& c = g_pGlobalState->config;

    c.enabled      = makeShared<Config::Values::CBoolValue>("plugin:spyhud:enabled", "Enable the HUD decorations", true);
    c.ticks        = makeShared<Config::Values::CBoolValue>("plugin:spyhud:ticks", "Draw hash ticks in the title bar", true);
    c.showClose    = makeShared<Config::Values::CBoolValue>("plugin:spyhud:show_close", "Draw the close box", true);
    c.leds         = makeShared<Config::Values::CBoolValue>("plugin:spyhud:leds", "Draw the status squares", true);
    c.barHeight    = makeShared<Config::Values::CIntValue>("plugin:spyhud:bar_height", "Title bar height", 22);
    c.textSize     = makeShared<Config::Values::CIntValue>("plugin:spyhud:text_size", "Title text size", 9);
    c.bracketLen   = makeShared<Config::Values::CIntValue>("plugin:spyhud:bracket_length", "Corner bracket length", 14);
    c.bracketThick = makeShared<Config::Values::CIntValue>("plugin:spyhud:bracket_thickness", "Corner bracket thickness", 2);
    c.panel        = makeShared<Config::Values::CColorValue>("plugin:spyhud:col.panel", "Focused bar color", 0xff34373b);
    c.panelInactive = makeShared<Config::Values::CColorValue>("plugin:spyhud:col.panel_inactive", "Unfocused bar color", 0xff2a2c2f);
    c.line         = makeShared<Config::Values::CColorValue>("plugin:spyhud:col.line", "Bevel highlight line", 0xff5a5e64);
    c.accent       = makeShared<Config::Values::CColorValue>("plugin:spyhud:col.accent", "Accent (yellow)", 0xfff2c400);
    c.dim          = makeShared<Config::Values::CColorValue>("plugin:spyhud:col.dim", "Dim / inactive accent", 0xff6b6f75);
    c.text         = makeShared<Config::Values::CColorValue>("plugin:spyhud:col.text", "Title text color", 0xffd8dadc);
    c.font         = makeShared<Config::Values::CStringValue>("plugin:spyhud:font", "Title font", "JetBrains Mono");


    HyprlandAPI::addConfigValueV2(PHANDLE, c.enabled);
    HyprlandAPI::addConfigValueV2(PHANDLE, c.ticks);
    HyprlandAPI::addConfigValueV2(PHANDLE, c.showClose);
    HyprlandAPI::addConfigValueV2(PHANDLE, c.leds);
    HyprlandAPI::addConfigValueV2(PHANDLE, c.barHeight);
    HyprlandAPI::addConfigValueV2(PHANDLE, c.textSize);
    HyprlandAPI::addConfigValueV2(PHANDLE, c.bracketLen);
    HyprlandAPI::addConfigValueV2(PHANDLE, c.bracketThick);
    HyprlandAPI::addConfigValueV2(PHANDLE, c.panel);
    HyprlandAPI::addConfigValueV2(PHANDLE, c.panelInactive);
    HyprlandAPI::addConfigValueV2(PHANDLE, c.line);
    HyprlandAPI::addConfigValueV2(PHANDLE, c.accent);
    HyprlandAPI::addConfigValueV2(PHANDLE, c.dim);
    HyprlandAPI::addConfigValueV2(PHANDLE, c.text);
    HyprlandAPI::addConfigValueV2(PHANDLE, c.font);

    for (auto& w : Desktop::windowState()->windows()) {
        if (w->isHidden() || !validMapped(w))
            continue;

        onNewWindow(w);
    }

    HyprlandAPI::reloadConfig();

    return {"spyhud", "SPYHUNTER-style HUD window decorations.", "wesley", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    for (auto& m : State::monitorState()->monitors())
        m->m_scheduledRecalc = true;

    g_pHyprRenderer->context().m_pass.removeAllOfType("CHudPassElement");
}
