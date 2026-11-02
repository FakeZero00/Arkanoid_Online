#pragma once
class Vec2D {
public:
    float x;
    float y;

    Vec2D(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

    // 벡터 덧셈 연산자 오버로딩 (위치 업데이트용)
    Vec2D operator+(const Vec2D& other) const;

    // 벡터 누적 덧셈 연산자
    Vec2D& operator+=(const Vec2D& other);

    // 벡터 스칼라 곱셈 연산자 (속도 * deltaTime용)
    Vec2D operator*(float scalar) const;
};