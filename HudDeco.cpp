#include "HudDeco.hpp"

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/state/FocusState.hpp>
#include <hyprland/src/desktop/state/WindowState.hpp>
#include <hyprland/src/desktop/state/ViewHitTester.hpp>
#include <hyprland/src/desktop/view/window/Window.hpp>
#include <hyprland/src/desktop/view/window/WindowPresentation.hpp>
#include <hyprland/src/layout/LayoutManager.hpp>
#include <hyprland/src/desktop/view/LayerSurface.hpp>
#include <hyprland/src/helpers/MiscFunctions.hpp>
#include <hyprland/src/managers/SeatManager.hpp>
#include <hyprland/src/managers/input/InputManager.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/protocols/LayerShell.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/state/MonitorState.hpp>

#include "HudPassElement.hpp"

#include <algorithm>
#include <cctype>

using namespace Render::GL;

static CHyprColor configColor(Config::INTEGER color) {
    return CHyprColor{static_cast<uint64_t>(color)};
}

static CHyprColor withAlpha(CHyprColor c, float a) {
    c.a *= a;
    return c;
}

CHudDeco::CHudDeco(PHLWINDOW pWindow) : IHyprWindowDecoration(pWindow) {
    m_pWindow = pWindow;

    const auto PMONITOR         = pWindow->m_monitor.lock();
    PMONITOR->m_scheduledRecalc = true;

    m_pMouseButtonCallback = Event::bus()->m_events.input.mouse.button.listen([&](IPointer::SButtonEvent e, Event::SCallbackInfo& info) { onMouseButton(info, e); });
    m_pMouseMoveCallback   = Event::bus()->m_events.input.mouse.move.listen([&](Vector2D c, Event::SCallbackInfo& info) { onMouseMove(c); });
}

CHudDeco::~CHudDeco() {
    std::erase(g_pGlobalState->decos, m_self);
}

SDecorationPositioningInfo CHudDeco::getPositioningInfo() {
    const auto                 HEIGHT  = g_pGlobalState->config.barHeight->value();
    const auto                 ENABLED = g_pGlobalState->config.enabled->value();

    SDecorationPositioningInfo info;
    info.policy         = DECORATION_POSITION_STICKY;
    info.edges          = DECORATION_EDGE_TOP;
    info.priority       = 5000;
    info.reserved       = true;
    info.desiredExtents = {{0, ENABLED ? HEIGHT : 0}, {0, 0}};
    return info;
}

void CHudDeco::onPositioningReply(const SDecorationPositioningReply& reply) {
    m_bAssignedBox = reply.assignedGeometry;
}

std::string CHudDeco::getDisplayName() {
    return "SpyHud";
}

eDecorationType CHudDeco::getDecorationType() {
    return DECORATION_CUSTOM;
}

eDecorationLayer CHudDeco::getDecorationLayer() {
    // OVER: the corner brackets sit on top of the border
    return DECORATION_LAYER_OVER;
}

uint64_t CHudDeco::getDecorationFlags() {
    return DECORATION_ALLOWS_MOUSE_INPUT | DECORATION_PART_OF_MAIN_WINDOW;
}

PHLWINDOW CHudDeco::getOwner() {
    return m_pWindow.lock();
}

void CHudDeco::updateWindow(PHLWINDOW) {
    damageEntire();
}

void CHudDeco::damageEntire() {
    g_pHyprRenderer->damageBox(outerBoxGlobal().expand(4));
}

void CHudDeco::onConfigReloaded() {
    m_textDirty = true;
    m_pTextTex  = nullptr;
    g_pDecorationPositioner->repositionDeco(this);
    damageEntire();
}

void CHudDeco::onFocusChange() {
    damageEntire();
}

bool CHudDeco::isFocused() {
    return m_pWindow.lock() == Desktop::focusState()->window();
}

CBox CHudDeco::assignedBoxGlobal() {
    if (!validMapped(m_pWindow))
        return {};

    CBox box = m_bAssignedBox;
    box.translate(g_pDecorationPositioner->getEdgeDefinedPoint(DECORATION_EDGE_TOP, m_pWindow.lock()));

    const auto PWORKSPACE      = m_pWindow->m_workspace;
    const auto WORKSPACEOFFSET = PWORKSPACE && !(m_pWindow->m_state & Desktop::View::WINDOW_STATE_PINNED) ? PWORKSPACE->m_renderOffset->value() : Vector2D();

    return box.translate(WORKSPACEOFFSET);
}

CBox CHudDeco::outerBoxGlobal() {
    if (!validMapped(m_pWindow))
        return {};

    const auto BAR = assignedBoxGlobal();
    const auto BS  = m_pWindow->presentation().borderSize();

    // bar spans the full window width and sits directly on top of the window
    return CBox{BAR.x - BS, BAR.y - BS, BAR.w + BS * 2, BAR.h + m_pWindow->size(Desktop::View::IGeometric::GEOMETRIC_CURRENT).y + BS * 2};
}

CBox CHudDeco::closeBoxLocal() {
    const auto H  = g_pGlobalState->config.barHeight->value();
    const auto W  = assignedBoxGlobal().w;
    const int  SZ = std::max<int>(8, H - 10);
    return CBox{W - SZ - 6.0, (H - SZ) / 2.0, sc<double>(SZ), sc<double>(SZ)};
}

Vector2D CHudDeco::cursorRelativeToBar() {
    return g_pInputManager->getMouseCoordsInternal() - assignedBoxGlobal().pos();
}

// ---------------------------------------------------------------- input

bool CHudDeco::inputIsValid() {
    if (!g_pGlobalState->config.enabled->value())
        return false;

    if (g_pSeatManager->m_seatGrab && !g_pSeatManager->m_seatGrab->accepts(m_pWindow->wlSurface()->resource()))
        return false;

    const auto MOUSE    = g_pInputManager->getMouseCoordsInternal();
    auto       PMONITOR = Desktop::focusState()->monitor();

    if (!PMONITOR)
        return false;

    Desktop::CViewHitTester hitTester{*Desktop::viewState()};

    const auto              WINDOWATCURSOR = hitTester.windowAt(MOUSE, Desktop::View::RESERVED_EXTENTS | Desktop::View::INPUT_EXTENTS | Desktop::View::ALLOW_FLOATING);
    const auto              window         = Desktop::focusState()->window();

    if (WINDOWATCURSOR != m_pWindow && m_pWindow != window)
        return false;

    PHLLS    foundSurface = nullptr;
    Vector2D surfaceCoords;

    hitTester.layerSurfaceAt(MOUSE, &PMONITOR->m_layerSurfaceLayers[ZWLR_LAYER_SHELL_V1_LAYER_TOP], &surfaceCoords, &foundSurface);
    if (foundSurface)
        return false;

    hitTester.layerSurfaceAt(MOUSE, &PMONITOR->m_layerSurfaceLayers[ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY], &surfaceCoords, &foundSurface);
    if (foundSurface)
        return false;

    return true;
}

void CHudDeco::onMouseButton(Event::SCallbackInfo& info, IPointer::SButtonEvent e) {
    if (!inputIsValid())
        return;

    const auto PWINDOW = m_pWindow.lock();

    if (e.state != WL_POINTER_BUTTON_STATE_PRESSED) {
        if (PWINDOW != Desktop::focusState()->window())
            return;

        if (m_bCancelledDown)
            info.cancelled = true;
        m_bCancelledDown = false;

        if (m_bDraggingThis) {
            g_layoutManager->endDragTarget();
            m_bDraggingThis = false;
        }
        m_bDragPending = false;
        return;
    }

    const auto COORDS = cursorRelativeToBar();
    const auto H      = g_pGlobalState->config.barHeight->value();

    if (!VECINRECT(COORDS, 0, 0, assignedBoxGlobal().w, H - 1)) {
        m_bDraggingThis = false;
        m_bDragPending  = false;
        return;
    }

    if (Desktop::focusState()->window() != PWINDOW)
        Desktop::focusState()->fullWindowFocus(PWINDOW, Desktop::FOCUS_REASON_CLICK);

    if (PWINDOW->isFloating())
        Desktop::windowState()->raise(PWINDOW);

    info.cancelled   = true;
    m_bCancelledDown = true;

    const auto CB = closeBoxLocal();
    if (g_pGlobalState->config.showClose->value() && VECINRECT(COORDS, CB.x, CB.y, CB.x + CB.w, CB.y + CB.h)) {
        PWINDOW->sendClose();
        return;
    }

    m_bDragPending = true;
}

void CHudDeco::onMouseMove(Vector2D) {
    if (validMapped(m_pWindow) && g_pGlobalState->config.enabled->value()) {
        const auto COORDS = cursorRelativeToBar();
        const auto H      = g_pGlobalState->config.barHeight->value();
        const auto CB     = closeBoxLocal();

        const bool BARHOVER   = VECINRECT(COORDS, 0, 0, assignedBoxGlobal().w, H);
        const bool CLOSEHOVER = BARHOVER && VECINRECT(COORDS, CB.x, CB.y, CB.x + CB.w, CB.y + CB.h);

        if (BARHOVER != m_barHovered || CLOSEHOVER != m_closeHovered) {
            m_barHovered   = BARHOVER;
            m_closeHovered = CLOSEHOVER;
            damageEntire();
        }
    }

    if (!m_bDragPending || !validMapped(m_pWindow))
        return;

    m_bDragPending = false;
    g_layoutManager->beginDragTarget(m_pWindow.lock()->layoutTarget(), MBIND_MOVE);
    m_bDraggingThis = true;
}

// ---------------------------------------------------------------- render

void CHudDeco::draw(Render::CRenderContext& ctx, PHLMONITOR pMonitor, const float& a, const Render::SWindowRenderPresentation& presentation) {
    if (!g_pGlobalState->config.enabled->value() || !validMapped(m_pWindow))
        return;

    const auto PWINDOW = m_pWindow.lock();

    if (!PWINDOW->m_ruleApplicator->decorate().valueOrDefault())
        return;

    Render::IHyprRenderer::addPassElement(ctx, makeUnique<CHudPassElement>(CHudPassElement::SHudData{this, a}));
}

void CHudDeco::renderTitle(int maxWidth, int scaledSize, const CHyprColor& col) {
    if (m_szLastTitle.empty() || maxWidth < 1) {
        m_pTextTex = nullptr;
        return;
    }

    m_pTextTex = g_pHyprRenderer->renderText(m_szLastTitle, col, scaledSize, false, g_pGlobalState->config.font->value(), maxWidth, 500);
}

void CHudDeco::renderPass(Render::CRenderContext& ctx, PHLMONITOR pMonitor, const float& a) {
    const auto PWINDOW = m_pWindow.lock();
    if (!PWINDOW)
        return;

    const auto& C = g_pGlobalState->config;

    const bool FOCUS = isFocused();
    if (FOCUS != m_lastFocus) {
        m_lastFocus = FOCUS;
        m_textDirty = true;
    }

    const float SCALE = pMonitor->m_scale;
    const int   H     = C.barHeight->value();

    if (H < 1)
        return;

    const auto PWORKSPACE      = PWINDOW->m_workspace;
    const auto WORKSPACEOFFSET = PWORKSPACE && !(PWINDOW->m_state & Desktop::View::WINDOW_STATE_PINNED) ? PWORKSPACE->m_renderOffset->value() : Vector2D();

    // ---- palette
    const CHyprColor COL_PANEL  = withAlpha(configColor(FOCUS ? C.panel->value() : C.panelInactive->value()), a);
    const CHyprColor COL_LINE   = withAlpha(configColor(C.line->value()), a);
    const CHyprColor COL_ACCENT = withAlpha(FOCUS ? configColor(C.accent->value()) : configColor(C.dim->value()), a);
    const CHyprColor COL_DIM    = withAlpha(configColor(C.dim->value()), a);
    const CHyprColor COL_DARK   = withAlpha(CHyprColor{0.09, 0.10, 0.11, 1.0}, a);
    const CHyprColor COL_TEXT   = configColor(C.text->value());

    // local helper: draw a rect in logical px relative to a base box (already in monitor-local scaled coords)
    const auto R = [&](const CBox& base, double x, double y, double w, double h, const CHyprColor& col) {
        CBox b = {base.x + x * SCALE, base.y + y * SCALE, w * SCALE, h * SCALE};
        b.round();
        if (b.w < 1)
            b.w = 1;
        if (b.h < 1)
            b.h = 1;
        g_pHyprOpenGL->renderRect(ctx, b, col, CHyprOpenGLImpl::SRectRenderData{.round = 0});
    };

    // ---- bar box (monitor-local, scaled)
    const auto DECOBOX = assignedBoxGlobal();
    CBox       bar     = {DECOBOX.x - pMonitor->m_position.x, DECOBOX.y - pMonitor->m_position.y, DECOBOX.w, DECOBOX.h};
    bar.translate(PWINDOW->presentation().floatingOffset()).scale(SCALE).round();

    if (bar.w < 1 || bar.h < 1)
        return;

    const double W = DECOBOX.w; // logical width

    g_pHyprOpenGL->scissor(ctx, bar);

    // body
    R(bar, 0, 0, W, H, COL_PANEL);
    // bevel: light line on top, dark line at the bottom (the "stacked plate" look)
    R(bar, 0, 0, W, 1, COL_LINE);
    R(bar, 0, H - 1, W, 1, COL_DARK);

    // yellow tab (top-left) + marker block
    R(bar, 0, 0, 38, 2, COL_ACCENT);
    R(bar, 6, (H - 8) / 2.0, 3, 8, COL_ACCENT);

    // ---- right side: LEDs + close box
    double rightEdge = W - 6;

    if (C.showClose->value()) {
        const auto CB   = closeBoxLocal();
        const auto HOV  = m_closeHovered;
        const auto COLX = HOV ? COL_ACCENT : COL_DIM;
        // outline
        R(bar, CB.x, CB.y, CB.w, 1, COLX);
        R(bar, CB.x, CB.y + CB.h - 1, CB.w, 1, COLX);
        R(bar, CB.x, CB.y, 1, CB.h, COLX);
        R(bar, CB.x + CB.w - 1, CB.y, 1, CB.h, COLX);
        // inner square (becomes solid on hover)
        const double PAD = 3;
        if (HOV)
            R(bar, CB.x + PAD, CB.y + PAD, CB.w - PAD * 2, CB.h - PAD * 2, COL_ACCENT);
        else
            R(bar, CB.x + CB.w / 2.0 - 1, CB.y + CB.h / 2.0 - 1, 2, 2, COLX);
        rightEdge = CB.x - 8;
    }

    if (C.leds->value()) {
        // three tiny status squares: one lit (accent), two dim
        for (int i = 0; i < 3; ++i) {
            const double X = rightEdge - 5 - i * 8;
            R(bar, X, (H - 5) / 2.0, 5, 5, i == 0 ? COL_ACCENT : COL_DARK);
            if (i != 0) {
                R(bar, X, (H - 5) / 2.0, 5, 1, COL_LINE);
            }
        }
        rightEdge -= 3 * 8 + 6;
    }

    // ---- title
    const int    SCALEDSIZE = std::round(C.textSize->value() * SCALE);
    const double TEXTX      = 16;
    const int    MAXW       = std::max(0, sc<int>((rightEdge - TEXTX - 8) * SCALE));

    const std::string TITLE = [&] {
        std::string t = PWINDOW->metadata().title();
        std::ranges::transform(t, t.begin(), [](unsigned char c) { return std::toupper(c); });
        return t;
    }();

    if (m_textDirty || TITLE != m_szLastTitle || !m_pTextTex || m_pTextTex->m_texID == 0 || SCALEDSIZE != m_iLastScaledTextSize || MAXW != m_iLastWidth) {
        m_szLastTitle         = TITLE;
        m_iLastScaledTextSize = SCALEDSIZE;
        m_iLastWidth          = MAXW;
        m_textDirty           = false;
        renderTitle(MAXW, SCALEDSIZE, FOCUS ? COL_TEXT : CHyprColor{COL_TEXT.r * 0.65, COL_TEXT.g * 0.65, COL_TEXT.b * 0.65, COL_TEXT.a});
    }

    double textEnd = TEXTX;
    if (m_pTextTex) {
        CBox tb = {bar.x + TEXTX * SCALE, bar.y + std::round((bar.h - m_pTextTex->m_size.y) / 2.0), m_pTextTex->m_size.x, m_pTextTex->m_size.y};
        g_pHyprOpenGL->renderTexture(ctx, m_pTextTex, tb, {.a = a});
        textEnd = TEXTX + m_pTextTex->m_size.x / SCALE;
    }

    // ---- hash ticks filling the free space between title and LEDs
    if (C.ticks->value()) {
        const double START = textEnd + 10;
        const double END   = rightEdge - 4;
        int          n     = 0;
        for (double x = START; x < END && n < 80; x += 5, ++n)
            R(bar, x, H - 6, 1, (n % 4 == 0) ? 4 : 2, COL_DIM);
    }

    g_pHyprOpenGL->scissor(ctx, nullptr);

    // ---- corner brackets around the whole plate (bar + window + border)
    {
        CBox outer = outerBoxGlobal();
        outer.translate(-pMonitor->m_position).translate(PWINDOW->presentation().floatingOffset()).scale(SCALE).round();

        const int LEN = C.bracketLen->value();
        const int T   = std::max<int>(1, C.bracketThick->value());

        const double OW = outer.w / SCALE;
        const double OH = outer.h / SCALE;

        // top-left stays under the yellow tab, so only use the dim colour for it when unfocused
        const CBox& o = outer;
        // TL
        R(o, 0, 0, LEN, T, COL_ACCENT);
        R(o, 0, 0, T, LEN, COL_ACCENT);
        // TR
        R(o, OW - LEN, 0, LEN, T, COL_ACCENT);
        R(o, OW - T, 0, T, LEN, COL_ACCENT);
        // BL
        R(o, 0, OH - T, LEN, T, COL_ACCENT);
        R(o, 0, OH - LEN, T, LEN, COL_ACCENT);
        // BR
        R(o, OW - LEN, OH - T, LEN, T, COL_ACCENT);
        R(o, OW - T, OH - LEN, T, LEN, COL_ACCENT);
    }

    if (m_iLastHeight != H) {
        m_iLastHeight = H;
        PWINDOW->layoutTarget()->recalc();
    }
}
