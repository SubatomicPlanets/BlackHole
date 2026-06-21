#pragma once
#include "vec3.h"

class Camera {
public:
	Vec3 position;
	float yaw, pitch;
	Vec3 front, up, right;
	float movement_speed = 2.0f;
	float mouse_sensitivity = 0.002f;

	Camera();
	void update_vectors();
	void process_mouse_movement(float dx, float dy);
	void process_keyboard_movement(float dt, bool w_key, bool s_key, bool a_key, bool d_key);
};