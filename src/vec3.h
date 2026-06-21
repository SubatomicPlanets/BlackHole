#pragma once
#include <cmath>

struct Vec3 {
	float x, y, z;
	Vec3(float x = 0.0f, float y = 0.0f, float z = 0.0f) : x(x), y(y), z(z) {}
	Vec3 operator+(const Vec3& v) const { return { x + v.x, y + v.y, z + v.z }; }
	Vec3 operator-(const Vec3& v) const { return { x - v.x, y - v.y, z - v.z }; }
	Vec3 operator*(float s) const { return { x * s, y * s, z * s }; }
	Vec3 operator-() const { return { -x, -y, -z }; }
	float dot(const Vec3& v) const { return x * v.x + y * v.y + z * v.z; }
	Vec3 cross(const Vec3& v) const {
		return { y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x };
	}
	float length_squared() const { return dot(*this); }
	float length() const { return sqrtf(length_squared()); }
	Vec3 normalize() const {
		float len = length();
		return (len > 1e-6f) ? *this * (1.0f / len) : *this;
	}
	Vec3 operator/(float s) const {
		if (s != 0.0f) {
			return { x / s, y / s, z / s };
		}
		return { 0.0f, 0.0f, 0.0f };
	}
	Vec3& operator+=(const Vec3& v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}
	Vec3& operator-=(const Vec3& v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}
};