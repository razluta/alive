#pragma once

#include "resourcemapper.hpp"
#include "core/component.hpp"

class TransformComponent;

class AnimationComponent final : public Component
{
public:
    DECLARE_COMPONENT(AnimationComponent);

public:
    void Deserialize(std::istream& is) override;

public:
    void Load() final;
    void Update();
    void Render(AbstractRenderer& rend) const;

public:
    void Change(const char* animationName);
    bool Complete() const;
    s32 FrameNumber() const;

public:
    bool mFlipX = false;
    bool mLoaded = false;
    std::unique_ptr<Animation> mAnimation;

private:
    ResourceLocator* mResourceLocator = nullptr;
    TransformComponent* mTransformComponent = nullptr;
}; 
