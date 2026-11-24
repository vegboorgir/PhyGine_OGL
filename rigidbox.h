#ifndef RIGIDBOX_H
#define RIGIDBOX_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct RigidBox {
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 velocity = glm::vec3(0.0f);
	glm::vec3 halfSize = glm::vec3(0.5f);
	glm::vec3 color = glm::vec3(1.0f);
	bool isStatic = false;
};
#endif
