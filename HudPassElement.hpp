#pragma once
#include <hyprland/src/render/pass/PassElement.hpp>

class CHudDeco;

class CHudPassElement : public IPassElement {
  public:
    struct SHudData {
        CHudDeco* deco = nullptr;
        float     a    = 1.F;
    };

    CHudPassElement(const SHudData& data_);
    virtual ~CHudPassElement() = default;

    virtual std::vector<UP<IPassElement>> draw(Render::CRenderContext& ctx) override;
    virtual bool                          needsLiveBlur(Render::CRenderContext&) override;
    virtual bool                          needsPrecomputeBlur(Render::CRenderContext&) override;
    virtual std::optional<CBox>           boundingBox(Render::CRenderContext& ctx) override;

    virtual const char*                   passName() override {
        return "CHudPassElement";
    }

    virtual ePassElementType type() override {
        return EK_CUSTOM;
    }

  private:
    SHudData data;
};
