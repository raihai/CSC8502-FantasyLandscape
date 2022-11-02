#pragma once

#include "../nclgl/OGLRenderer.h"
#include "../nclgl/Camera.h"
#include "../nclgl/SceneNode.h"
#include "CubeRobot.h"

class Renderer : public OGLRenderer {
public: 
	Renderer(Window& parent);
	~Renderer(void);

	void UpdateScene(float dt) override;
	void RenderScene() override;

protected:
	void DrawNode(SceneNode* n);
	void DrawNode2(SceneNode* n);

	SceneNode* root;
	SceneNode* sec;

	GLuint textures;

	Camera* camera;

	Mesh* cube;
	Mesh* shape;

	Shader* shader;
	Shader* secShader;
};
