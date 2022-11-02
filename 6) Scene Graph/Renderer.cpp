#include "Renderer.h"

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	cube = Mesh::LoadFromMeshFile("OffsetCubeY.msh");
	shape = Mesh::LoadFromMeshFile("Capsule.msh");
	

	camera = new Camera();

	shader = new Shader("SceneVertex.glsl", "SceneFragment.glsl");

	secShader = new Shader("basicVertex.glsl", "colourFragment.glsl");

	if (!shader->LoadSuccess() || !secShader->LoadSuccess()) {
		return;
	}

	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);

	camera->SetPosition(Vector3(0, 30, 175));


	root = new SceneNode();
	root->AddChild(new CubeRobot(cube));

	sec = new SceneNode();
	sec->SetTransform(Matrix4::Translation(Vector3(40, 0, 0)));
	sec->AddChild(new CubeRobot(shape));
	
	glEnable(GL_DEPTH_TEST);
	init = true;
}

Renderer ::~Renderer(void) {
	delete root;
	delete sec;
	delete shader;
	delete secShader;
	delete camera;
	delete cube;

}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();
	root->Update(dt);
	sec->Update(dt);
}


void Renderer::RenderScene() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	BindShader(shader);
	UpdateShaderMatrices();

	glUniform1i(glGetUniformLocation(shader -> GetProgram(),
		"diffuseTex"), 1);

	DrawNode(root);
	DrawNode(sec);
	
}

void Renderer::DrawNode(SceneNode* n) {
	
		if (n->GetMesh()) {
			Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());

			glUniformMatrix4fv(glGetUniformLocation(shader->GetProgram(),
				"modelMatrix"), 1, false, model.values);

			glUniform4fv(glGetUniformLocation(shader->GetProgram(), "nodeColour"), 1, (float*)&n->GetColour());

			glUniform1i(glGetUniformLocation(shader->GetProgram(), "useTexture"), 0);

			n->Draw(*this);
		}

		for (vector<SceneNode*>::const_iterator
			i = n->GetChildIteratorStart();
			i != n->GetChildIteratorEnd(); ++i)
		{
			DrawNode(*i);
		}


}