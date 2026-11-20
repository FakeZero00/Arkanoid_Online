#pragma once

#include "Vec2D.h"

class Object {
public:
    Vec2D position;

    virtual ~Object() = default;
    virtual void Update(float) {}
};

class Ball : public Object {
public:
    explicit Ball(Vec2D initialDirection) : direction(initialDirection) {}

    void Update(float deltaTime) override {
        constexpr float speed = 10.0f;
        position += direction * speed * deltaTime;
    }

private:
    Vec2D direction;
};

class Bous : public Object {
public:
    void Update(float) override {}
};
