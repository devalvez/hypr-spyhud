#pragma once

#define WLR_USE_UNSTABLE

#include <hyprland/src/render/decorations/IHyprWindowDecoration.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/render/gl/GLTexture.hpp>
#include <hyprland/src/devices/IPointer.hpp>
#include <hyprland/src/helpers/time/Time.hpp>
#include <hyprland/src/helpers/signal/Signal.hpp>
#include "globals.hpp"

#define private public
#include <hyprland/src/managers/input/InputManager.hpp>
#undef private

namespace Event {
    struct SCallbackInfo;
}

class CHudDeco : public IHyprWindowDecoration {
  public:
    CHudDeco(PHLWINDOW);
    virtual ~CHudDeco();

    virtual SDecorationPositioningInfo getPositioningInfo();
    virtual void                       onPositioningReply(const SDecorationPositioningReply& reply);
    virtual void                       draw(Render::CRenderContext& ctx, PHLMONITOR monitor, const float& a, const Render::SWindowRenderPresentation& presentation);
    virtual eDecorationType            getDecorationType();
    virtual void                       updateWindow(PHLWINDOW);
    virtual void                       damageEntire();
    virtual eDecorationLayer           getDecorationLayer();
    virtual uint64_t                   getDecorationFlags();
    virtual std::string                getDisplayName();

    PHLWINDOW                          getOwner();
    void                               onConfigReloaded();
    void                               onFocusChange();

    // box of bar + window + border, global coords
    CBox                               outerBoxGlobal();
    CBox                               assignedBoxGlobal();

    void                               renderPass(Render::CRenderContext& ctx, PHLMONITOR, const float& a);

    WP<CHudDeco>                       m_self;

  private:
    PHLWINDOWREF         m_pWindow;
    CBox                 m_bAssignedBox;

    SP<Render::ITexture> m_pTextTex;
    std::string          m_szLastTitle;
    bool                 m_lastFocus          = false;
    bool                 m_textDirty          = true;
    int                  m_iLastScaledTextSize = 0;
    int                  m_iLastWidth         = 0;
    int                  m_iLastHeight        = 0;

    bool                 m_closeHovered = false;
    bool                 m_barHovered   = false;
    bool                 m_bDraggingThis = false;
    bool                 m_bDragPending  = false;
    bool                 m_bCancelledDown = false;

    bool                 isFocused();
    CBox                 closeBoxLocal(); // relative to bar, logical px
    bool                 inputIsValid();
    Vector2D             cursorRelativeToBar();

    void                 renderTitle(int maxWidth, int scaledSize, const CHyprColor& col);

    void                 onMouseButton(Event::SCallbackInfo& info, IPointer::SButtonEvent e);
    void                 onMouseMove(Vector2D coords);

    CHyprSignalListener  m_pMouseButtonCallback;
    CHyprSignalListener  m_pMouseMoveCallback;
};
