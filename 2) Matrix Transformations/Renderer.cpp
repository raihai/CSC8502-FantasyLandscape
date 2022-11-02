#include "Renderer.h"

Renderer::Renderer(Window &parent) : OGLRenderer(parent) {
	camera = new Camera;
	triangle = Mesh::GenerateTriangle();

	matrixShader = new Shader("MatrixVertex.glsl", "colourFragment.glsl");

	if (!matrixShader->LoadSuccess()) {
		return;
	}

	init = true;
	SwitchToOrthographic();
}

Renderer::~Renderer(void) {
	delete triangle;
	delete matrixShader;
}

void Renderer::SwitchToPerspective() {
	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, fov);
}

void Renderer::SwitchToOrthographic() {
	projMatrix = Matrix4::Orthographic(-1.0f, 10000.0f, width / 2.0f, -width / 2.0f, height / 2.0f, -height / 2.0f);
}

void Renderer::RenderScene() 
{
	glClear(GL_COLOR_BUFFER_BIT);

	BindShader(matrixShader);

	glUniformMatrix4fv(glGetUniformLocation(matrixShader->GetProgram(), "projMatrix"), 1, false, projMatrix.values);

	glUniformMatrix4fv(glGetUniformLocation(matrixShader->GetProgram(), "viewMatrix"), 1, false, viewMatrix.values);

	for (int i = 0; i < 3; i++) {
		Vector3 tempPos = position;;
		tempPos.z += (i * 500.0f);
		tempPos.x -= (i * 100.0f);
		tempPos.y -= (i * 100.0f);

		modelMatrix = Matrix4::Translation(tempPos) * Matrix4::Rotation(rotation, Vector3(0, 1, 0)) * Matrix4::Scale(Vector3(scale, scale, scale)) ;

		glUniformMatrix4fv(glGetUniformLocation(matrixShader->GetProgram(), "modelMatrix"), 1, false, modelMatrix.values);
		triangle->Draw();
								
	}
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix() ;
}

// 1. Changing the order of the multiplication changes the position the triangles.
// 2. When drawing HUD, orthographic view is better as the model's scale remains 1 to 1 while perspective will scale down 
//	 model depending on the distance from the camera.


//2B.1. Multiplying the view matrix with a sacle matrix affects the model matrix too. 
//2B.2 Changing the order of rotations in the BVM function changes how the rotation occurs in the respect to the camera position.


