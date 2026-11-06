#include "Vec2D.h"

//Vec2D::Vec2D(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

// 벡터 덧셈 연산자 오버로딩 (위치 업데이트용)
Vec2D Vec2D::operator+(const Vec2D& other) const {
    return Vec2D(x + other.x, y + other.y);
}

// 벡터 누적 덧셈 연산자
Vec2D& Vec2D::operator+=(const Vec2D& other) {
    x += other.x;
    y += other.y;
    return *this;
}

Vec2D Vec2D::operator*(float scalar) const {
    return Vec2D(x * scalar, y * scalar);
}