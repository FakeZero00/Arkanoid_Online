#pragma once

#include "Vec2D.h"
#include <cstdint>

class Object {
public:
    static unsigned int objCounter; // 오브젝트 고유 ID를 위한 카운터
    unsigned int id;
    std::uint32_t sequence = 0;
    Vec2D position;

    Object() {
        id = objCounter++;
    }

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
