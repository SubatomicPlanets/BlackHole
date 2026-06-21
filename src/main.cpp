#include "camera.h"
#include "vec3.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <sstream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Constants
const int render_width = 800;
const int render_height = 600;
const int window_width = 800;
const int window_height = 600;

// Shaders
const char* vertexShaderSource = R"(
#version 430
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 texCoord;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    texCoord = aTexCoord;
}
)";

const char* fragmentShaderSource = R"(
#version 430
in vec2 texCoord;
out vec4 FragColor;

uniform sampler2D screenTexture;

void main() {
    FragColor = texture(screenTexture, texCoord);
}
)";

std::string loadStringFile(const std::string& path) {
	// Read text file as string
	std::ifstream f(path);
	std::stringstream s;
	s << f.rdbuf();
	return s.str();
}

GLuint loadVerticalCubemap(const char* filepath) {
	// Load a cubemap image
	int width, height, nrComponents;
	float* data = stbi_loadf(filepath, &width, &height, &nrComponents, 3);
	if (!data) return 0;

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	// Loop through the 6 vertical segments of the image
	int faceSize = width;
	for (unsigned int i = 0; i < 6; ++i) {
		float* faceData = data + (i * faceSize * faceSize * 3);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, faceSize, faceSize, 0, GL_RGB, GL_FLOAT, faceData);
	}

	// Hardware sampling state optimization
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	stbi_image_free(data);
	return textureID;
}

int main() {
	// GLFW and glad setup
	if (!glfwInit()) return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(window_width, window_height, "BlackHole", nullptr, nullptr);
	if (!window) { glfwTerminate(); return -1; }

	glfwMakeContextCurrent(window);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { return -1; }

	// Compute shader setup
	std::string computeSrc = loadStringFile("raytrace.comp");
	const char* computeSrcPtr = computeSrc.c_str();
	GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(computeShader, 1, &computeSrcPtr, NULL);
	glCompileShader(computeShader);

	GLuint computeProgram = glCreateProgram();
	glAttachShader(computeProgram, computeShader);
	glLinkProgram(computeProgram);

	// Render texture
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, render_width, render_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Quad for rendering
	float quadVertices[] = {
		// positions   // tex coords
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f
	};
	unsigned int quadIndices[] = { 0, 1, 2, 2, 3, 0 };

	GLuint VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Render shaders
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	GLuint renderProgram = glCreateProgram();
	glAttachShader(renderProgram, vertexShader);
	glAttachShader(renderProgram, fragmentShader);
	glLinkProgram(renderProgram);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// Camera setup
	Camera camera;

	// Sky cubemap
	GLuint skyCubemap = loadVerticalCubemap("sky.hdr");

	// Window loop
	double last_time = glfwGetTime();
	double last_mouse_x, last_mouse_y;
	glfwGetCursorPos(window, &last_mouse_x, &last_mouse_y);
	while (!glfwWindowShouldClose(window)) {
		// Delta time
		double current_time = glfwGetTime();
		float delta_time = static_cast<float>(current_time - last_time);
		last_time = current_time;

		// Poll events
		glfwPollEvents();
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			break;
		}

		// Camera rotation
		double mouse_x, mouse_y;
		glfwGetCursorPos(window, &mouse_x, &mouse_y);
		float delta_x = static_cast<float>(mouse_x - last_mouse_x);
		float delta_y = static_cast<float>(mouse_y - last_mouse_y);
		last_mouse_x = mouse_x;
		last_mouse_y = mouse_y;
		camera.process_mouse_movement(delta_x, delta_y);

		// Camera movement
		bool w_key = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
		bool s_key = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
		bool a_key = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
		bool d_key = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
		camera.process_keyboard_movement(delta_time, w_key, s_key, a_key, d_key);

		// Run compute shader
		glUseProgram(computeProgram);
		glUniform3f(glGetUniformLocation(computeProgram, "cameraPos"), camera.position.x, camera.position.y, camera.position.z);
		glUniform3f(glGetUniformLocation(computeProgram, "cameraFront"), camera.front.x, camera.front.y, camera.front.z);
		glUniform3f(glGetUniformLocation(computeProgram, "cameraUp"), camera.up.x, camera.up.y, camera.up.z);
		glUniform3f(glGetUniformLocation(computeProgram, "cameraRight"), camera.right.x, camera.right.y, camera.right.z);
		glUniform2f(glGetUniformLocation(computeProgram, "resolution"), render_width, render_height);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyCubemap);

		glBindImageTexture(0, tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
		int groups_x = (render_width + 15) / 16;
		int groups_y = (render_height + 15) / 16;
		glDispatchCompute(groups_x, groups_y, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		// Render quad
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(renderProgram);
		glBindTexture(GL_TEXTURE_2D, tex);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// Display
		glfwSwapBuffers(window);
	}

	// Cleanup
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteProgram(renderProgram);
	glDeleteProgram(computeProgram);
	glDeleteShader(computeShader);
	glDeleteTextures(1, &tex);

	// Stop
	glfwTerminate();
	return 0;
}