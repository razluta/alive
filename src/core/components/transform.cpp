#include "mapobject.hpp"
#include "core/components/transform.hpp"

DEFINE_COMPONENT(TransformComponent);

void TransformComponent::Set(float xPos, float yPos)
{
    mXPos = xPos;
    mYPos = yPos;
}

void TransformComponent::SetX(float xPos)
{
    mXPos = xPos;
}

void TransformComponent::SetY(float yPos)
{
    mYPos = yPos;
}
float TransformComponent::GetX() const
{
    return mXPos;
}
float TransformComponent::GetY() const
{
    return mYPos;
}

void TransformComponent::Add(float xAmount, float yAmount)
{
    mXPos += xAmount;
    mYPos += yAmount;
}

void TransformComponent::AddX(float xAmount)
{
    mXPos += xAmount;
}

void TransformComponent::AddY(float yAmount)
{
    mYPos += yAmount;
}

void TransformComponent::SnapXToGrid()
{
    mXPos = ::SnapXToGrid(mXPos);
}