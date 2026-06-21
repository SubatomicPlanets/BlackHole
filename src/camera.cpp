#include "camera.h"
#include <cmath>

const float PI = 3.1415927f;

Camera::Camera() : position(0.0f, 1.0f, -6.0f), yaw(PI / 2.0f), pitch(0.0f) {
	update_vectors();
}

void Camera::update_vectors() {
	// Update front, right, and up vectors
	float cy = cosf(yaw), sy = sinf(yaw);
	float cp = cosf(pitch), sp = sinf(pitch);
	front = Vec3(cy * cp, sp, sy * cp).normalize();
	right = Vec3(0.0f, 1.0f, 0.0f).cross(front).normalize();
	up = front.cross(right).normalize();
}

void Camera::process_mouse_movement(float dx, float dy) {
	// Update yaw and pitch and automatically call update_vectors
	yaw -= dx * mouse_sensitivity;
	pitch -= dy * mouse_sensitivity;
	pitch = fmaxf(-89.0f * PI / 180.0f, fminf(89.0f * PI / 180.0f, pitch));
	update_vectors();
}

void Camera::process_keyboard_movement(float dt, bool w_key, bool s_key, bool a_key, bool d_key) {
	// Update position
	float vel = movement_speed * dt;
	Vec3 move;
	if (w_key) move = move + front;
	if (s_key) move = move - front;
	if (a_key) move = move - right;
	if (d_key) move = move + right;
	position = position + move.normalize() * vel;
}