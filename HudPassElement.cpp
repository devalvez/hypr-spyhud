#include "HudPassElement.hpp"
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include "HudDeco.hpp"

CHudPassElement::CHudPassElement(const CHudPassElement::SHudData& data_) : data(data_) {
    ;
}

std::vector<UP<IPassElement>> CHudPassElement::draw(Render::CRenderContext& ctx) {
    data.deco->renderPass(ctx, ctx.m_data.pMonitor.lock(), data.a);
    return {};
}

bool CHudPassElement::needsLiveBlur(Render::CRenderContext&) {
    return false;
}

bool CHudPassElement::needsPrecomputeBlur(Render::CRenderContext&) {
    return false;
}

std::optional<CBox> CHudPassElement::boundingBox(Render::CRenderContext& ctx) {
    // expand a bit so occlusion culling never eats the brackets that sit on the border
    return data.deco->outerBoxGlobal().translate(-ctx.m_data.pMonitor->m_position).expand(10);
}
