#include "Renderer.h"
#include "../nclgl/Light.h"
#include "../nclgl/Heightmap.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Camera.h"
#include "../nclgl/MeshMaterial.h"
#include "../nclgl//MeshAnimation.h"
#include <algorithm>
#include <random>
#include <cstdlib>

#define SHADOWSIZE 2048
const int POST_PASSES = 10;


Renderer::Renderer(Window& parent) : OGLRenderer(parent) {

	//glEnable(GL_CULL_FACE);

	quad = Mesh::GenerateQuad();
	postQuad = Mesh::GenerateQuad();
	tree = Mesh::LoadFromMeshFile("Tree10_3.msh");
	material = new MeshMaterial("Tree10_3.mat");

	charMesh = Mesh::LoadFromMeshFile("Ch50_nonPBR.msh");
	charAnim = new MeshAnimation("ch50 Dance.anm");
	charMat = new MeshMaterial("Ch50_nonPBR.mat");

	heightMap = new HeightMap(TEXTUREDIR"finalTerrain.png");

	waterTex = SOIL_load_OGL_texture(TEXTUREDIR "water.TGA", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthTex = SOIL_load_OGL_texture(TEXTUREDIR "Barren Reds.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthBump = SOIL_load_OGL_texture(TEXTUREDIR "Barren RedsDOT3.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	cubeMap = SOIL_load_OGL_cubemap(TEXTUREDIR "rusted_west.jpg", TEXTUREDIR "rusted_east.jpg", TEXTUREDIR "rusted_up.jpg", TEXTUREDIR "rusted_down.jpg",
		TEXTUREDIR "rusted_south.jpg", TEXTUREDIR "rusted_north.jpg", SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	if (!earthTex || !earthBump || !cubeMap || !waterTex) {
		return;
	}

	SetTextureRepeating(earthTex, true);
	SetTextureRepeating(earthBump, true);
	SetTextureRepeating(waterTex, true);

	reflectShader = new Shader("reflectVertex.glsl", "reflectFragment.glsl");
	skyboxShader = new Shader("skyboxVertex.glsl", "skyboxFragment.glsl");
	lightShader = new Shader("BumpVertex.glsl", "BumpFragment.glsl");
	treeShader = new Shader("SceneVertex.glsl", "SceneFragment.glsl");
	charShader = new Shader("SkinningVertex.glsl", "TexturedFragment.glsl");

	sceneShader = new Shader("TexturedVertex.glsl", "TexturedFragment.glsl");
	processShader = new Shader("TexturedVertex.glsl", "processfrag.glsl");


	if (!reflectShader->LoadSuccess() ||
		!skyboxShader->LoadSuccess() ||
		!lightShader->LoadSuccess() || !treeShader->LoadSuccess() || !charShader->LoadSuccess() || !sceneShader->LoadSuccess() || !processShader->LoadSuccess()) {
		return;
	}

	Vector3 heightmapSize = heightMap -> GetHeightmapSize();
	camera = new Camera(-45.0f, 0.0f, heightmapSize * Vector3(0.5f, 5.0f, 0.5f));
	light = new Light(heightmapSize * Vector3(0.5f, 1.5f, 0.5f), Vector4(1, 1, 1, 1), heightmapSize.x);
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	root = new SceneNode();

	for (int i = 0; i < tree->GetSubMeshCount(); ++i) {
	 const MeshMaterialEntry * matEntry =
		 material->GetMaterialForLayer(i);
	
		const string * filename = nullptr;
		matEntry->GetEntry("Diffuse", &filename);
		string path = TEXTUREDIR + *filename;
		GLuint texID = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
		matTextures.emplace_back(texID);
	}

	srand(time(NULL));
	for (int i = 0; i < 10; ++i) {
		SceneNode* s = new SceneNode();
		s->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));

		s->SetTransform(Matrix4::Translation(Vector3(rand() % int(heightmapSize.x), 100.0f, rand() % int(heightmapSize.z))));
		s->SetModelScale(Vector3(20.0f, 20.0f, 20.0f));
		//s->SetBoundingRadius(10.0f);
		s->SetMesh(tree);

		for (int i = 0; i < tree->GetSubMeshCount(); ++i)
		{
			s->SetTexture(matTextures[i]);
			SetTextureRepeating(matTextures[i], true);
		}
		root->AddChild(s);
	}

	root->Update(0);

	for (int i = 0; i < charMesh->GetSubMeshCount(); ++i) {
		const MeshMaterialEntry* matEntry =
			charMat->GetMaterialForLayer(i);

		const string* filename = nullptr;
		matEntry->GetEntry("Diffuse", &filename);
		string path = TEXTUREDIR + *filename;
		GLuint texID = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
		charTextures.emplace_back(texID);
	}

	glGenTextures(1, &bufferDepthTex);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

	for (int i = 0; i < 2; ++i) {
		glGenTextures(1, &bufferColourTex[i]);
		glBindTexture(GL_TEXTURE_2D, bufferColourTex[i]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}

	glGenFramebuffers(1, &bufferFBO); // We ’ll render the scene into this
	glGenFramebuffers(1, &processFBO); // And do post processing in this
	
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
	GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[0], 0); // We can check FBO attachment success using this command !
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
		GL_FRAMEBUFFER_COMPLETE || !bufferDepthTex || !bufferColourTex[0]) {
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	waterRotate = 0.0f;
	waterCycle = 0.0f;
	currentFrame = 0;
	frameTime = 0.0f;
	init = true;
}

Renderer ::~Renderer(void) {
	delete camera;
	delete heightMap;
	delete quad;
	delete reflectShader;
	delete skyboxShader;
	delete lightShader;
	delete treeShader;
	delete charShader;
	delete light;
	delete root;
	delete tree;
	delete material;
	delete charMat;

	delete sceneShader;
	delete processShader;
	delete postQuad;

	glDeleteTextures(2, bufferColourTex);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteFramebuffers(1, &bufferFBO);
	glDeleteFramebuffers(1, &processFBO);

}

void Renderer::UpdateScene(float dt) {
	camera -> UpdateCamera(dt);
	viewMatrix = camera -> BuildViewMatrix();
	waterRotate += dt * 2.0f; //2 degrees a second
	waterCycle += dt * 0.25f; // 10 units a second
	frameFrustum.FromMatrix(projMatrix * viewMatrix);

	frameTime -= dt;
	while (frameTime < 0.0f) {
		currentFrame = (currentFrame + 1) % charAnim->GetFrameCount();
		frameTime += 1.0f / charAnim->GetFrameRate();
	}
	root->Update(dt);

}


void Renderer::RenderScene() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	DrawScene();
	DrawPostProcess();
	PresentScene();

	DrawSkybox();
	DrawHeightmap();
	//DrawCharAnim();
	BuildNodeLists(root);
	SortNodeLists();
	DrawNodes();
	ClearNodeLists();
	DrawWater();
}



void Renderer::PresentScene()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	BindShader(sceneShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
	glUniform1i(glGetUniformLocation(sceneShader -> GetProgram(), "diffuseTex"), 0);
	postQuad -> Draw();

}

void Renderer::DrawPostProcess()
{
	glBindFramebuffer(GL_FRAMEBUFFER, processFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[1], 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	BindShader(processShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);

	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(processShader->GetProgram(), "sceneTex"), 0);
	for (int i = 0; i < POST_PASSES; ++i) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, bufferColourTex[1], 0);
		glUniform1i(glGetUniformLocation(processShader->GetProgram(), "isVertical"), 0);

		glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
		postQuad->Draw();
		// Now to swap the colour buffers , and do the second blur pass
		glUniform1i(glGetUniformLocation(processShader -> GetProgram(), "isVertical"), 1);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, bufferColourTex[0], 0);
		glBindTexture(GL_TEXTURE_2D, bufferColourTex[1]);
		postQuad->Draw();

	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::DrawScene()
{
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT |
	GL_STENCIL_BUFFER_BIT);
	
	BindShader(sceneShader);
	projMatrix = Matrix4::Perspective(1.0f, 10000.0f,(float)width / (float)height, 45.0f);
	UpdateShaderMatrices();
	glUniform1i(glGetUniformLocation(sceneShader -> GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, earthTex);
	heightMap -> Draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void Renderer::DrawCharAnim()
{
	BindShader(charShader);
	glUniform1i(glGetUniformLocation(charShader->GetProgram(), "diffuseTex"), 0);
	UpdateShaderMatrices();

	vector<Matrix4> frameMatrices;
	const Matrix4* invBindPose = charMesh->GetInverseBindPose();
	const Matrix4* frameData = charAnim->GetJointData(currentFrame);
	Vector3 hSize = heightMap->GetHeightmapSize();
	Matrix4 model = Matrix4::Translation(Vector3(int(hSize.x/2), 100.0f, int(hSize.z/2))) * Matrix4::Scale(Vector3(100.0f, 100.0f, 100.0f));

	glUniformMatrix4fv(glGetUniformLocation(charShader->GetProgram(), "modelMatrix"), 1, false, model.values);

	for (unsigned int i = 0; i < charMesh->GetJointCount(); ++i)
	{
		frameMatrices.emplace_back(frameData[i] * invBindPose[i]);
	}

	int j = glGetUniformLocation(charShader->GetProgram(),"joints");
	glUniformMatrix4fv(j, frameMatrices.size(), false, (float*)frameMatrices.data());

	for (int i = 0; i < charMesh->GetSubMeshCount(); ++i)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, charTextures[i]);
		charMesh->DrawSubMesh(i);
	}
}

void Renderer::DrawNodes()
{
	for (const auto& i : nodeList) {
		DrawNode(i);
	}
	for (const auto& i : transparentNodeList) {
		DrawNode(i);
	}
}

void Renderer::DrawNode(SceneNode* n)
{
	if (n->GetMesh())
	{
		BindShader(treeShader);

		glUniform1i(glGetUniformLocation(treeShader->GetProgram(), "diffuseTex"), 0);
		glUniform4fv(glGetUniformLocation(treeShader->GetProgram(), "nodeColour"), 1, (float*)&n->GetColour());

		UpdateShaderMatrices();

		for (int i = 0; i < tree->GetSubMeshCount(); ++i)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, matTextures[i]);
			glUniform1i(glGetUniformLocation(treeShader->GetProgram(), "useTexture"), matTextures[i]);
		
			Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
			glUniformMatrix4fv(glGetUniformLocation(treeShader->GetProgram(), "modelMatrix"), 1, false, model.values);
	
			MeshShaderMatrices();
			tree->DrawSubMesh(i);
		}
	}
}

void Renderer::BuildNodeLists(SceneNode* from)
{
	if (frameFrustum.InsideFrustum(*from)) {
		Vector3 dir = from->GetWorldTransform().GetPositionVector() - camera->GetPosition();
		from->SetCameraDistance(Vector3::Dot(dir, dir));

		if (from->GetColour().w < 1.0f) {
			transparentNodeList.push_back(from);
		}
		else {
			nodeList.push_back(from);
		}
	}
	for(vector <SceneNode*>::const_iterator i = from->GetChildIteratorStart();
		i != from->GetChildIteratorEnd(); ++i) {
		BuildNodeLists((*i));
	}
}

void Renderer::SortNodeLists()
{
	std::sort(transparentNodeList.rbegin(), //note the r!
	transparentNodeList.rend(), //note the r!
	SceneNode::CompareByCameraDistance);
	std::sort(nodeList.begin(),
	nodeList.end(),
	SceneNode::CompareByCameraDistance);
}

void Renderer::ClearNodeLists()
{
	transparentNodeList.clear();
	nodeList.clear();
}



void Renderer::DrawSkybox() {
	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);
	UpdateShaderMatrices();
	quad->Draw();

	glDepthMask(GL_TRUE);
}


void Renderer::DrawHeightmap() {
	BindShader(lightShader);
	SetShaderLight(*light);
	glUniform3fv(glGetUniformLocation(lightShader -> GetProgram(),
	"cameraPos"), 1, (float*)&camera -> GetPosition());
	
	glUniform1i(glGetUniformLocation(lightShader -> GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, earthTex);

	glUniform1i(glGetUniformLocation(lightShader -> GetProgram(), "bumpTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, earthBump);

	modelMatrix.ToIdentity(); // New !
	textureMatrix.ToIdentity(); // New !
	
	UpdateShaderMatrices();
	heightMap -> Draw();
	
}

void Renderer::DrawWater() {
	BindShader(reflectShader);

	glUniform3fv(glGetUniformLocation(reflectShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "cubeTex"), 2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, waterTex);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	Vector3 hSize = heightMap -> GetHeightmapSize();
	modelMatrix = Matrix4::Translation(hSize * 0.5f) * Matrix4::Scale(hSize * 0.5f) * Matrix4::Rotation(90, Vector3(1, 0, 0));

	

	textureMatrix = Matrix4::Translation(Vector3(waterCycle, 0.0f, waterCycle)) * Matrix4::Scale(Vector3(10, 10, 10)) * Matrix4::Rotation(waterRotate, Vector3(0, 0, 1));
	
	UpdateShaderMatrices();
	// SetShaderLight (* light ); // No lighting in this shader !
	quad -> Draw();
}