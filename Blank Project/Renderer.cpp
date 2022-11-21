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

const int POST_PASSES = 10;
const float ROTATE_SPEED = 2.5f;


Renderer::Renderer(Window& parent) : OGLRenderer(parent) {

	quad = Mesh::GenerateQuad();
	
	tree = Mesh::LoadFromMeshFile("Tree10_3.msh");
	material = new MeshMaterial("Tree10_3.mat");

	charMesh = Mesh::LoadFromMeshFile("Ch45_nonPBR.msh");
	charAnim = new MeshAnimation("chTaunt.anm");
	charMat = new MeshMaterial("Ch45_nonPBR.mat");

	heightMap = new HeightMap(TEXTUREDIR"finalTerrain.png");

	waterTex = SOIL_load_OGL_texture(TEXTUREDIR "water.TGA", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	earthTex = SOIL_load_OGL_texture(TEXTUREDIR "ground.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	grassTex = SOIL_load_OGL_texture(TEXTUREDIR "grass.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	rockTex = SOIL_load_OGL_texture(TEXTUREDIR "Barren Reds.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	rockBump = SOIL_load_OGL_texture(TEXTUREDIR "normal1.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	cubeMap = SOIL_load_OGL_cubemap(TEXTUREDIR "fluffballdayleft.png", TEXTUREDIR "fluffballdayright.png", TEXTUREDIR "fluffballdaytop.png", TEXTUREDIR"fluffballdaybottom.png",
		TEXTUREDIR "fluffballdayfront.png", TEXTUREDIR "fluffballdayback.png",SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	if (!earthTex || !rockTex || !cubeMap || !waterTex || !grassTex || !rockBump) {
		return;
	}

	SetTextureRepeating(earthTex, true);
	SetTextureRepeating(grassTex, true);
	SetTextureRepeating(rockTex, true);
	SetTextureRepeating(waterTex, true);
	SetTextureRepeating(rockBump, true);

	fogColour = new Vector4(0.68f, 0.71f, 0.79f, 1.0);

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
	light = new Light(heightmapSize * Vector3(0.5f, 2.0f, 0.5f), Vector4(1, 1, 1, 1), heightmapSize.x);
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	root = new SceneNode();

	for (int i = 0; i < tree->GetSubMeshCount(); ++i) {
		const MeshMaterialEntry * matEntry = material->GetMaterialForLayer(i);	
		const string * filename = nullptr;
		matEntry->GetEntry("Diffuse", &filename);
		string path = TEXTUREDIR + *filename;
		GLuint texID = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
		matTextures.emplace_back(texID);
	}

	srand(time(NULL));
	for (int i = 0; i < 15; ++i) {
		SceneNode* s = new SceneNode();
		s->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));

		s->SetTransform(Matrix4::Translation(Vector3(rand() % int(heightmapSize.x * 0.8f), 100.0f, rand() % int(heightmapSize.z * 0.3f))));
		s->SetModelScale(Vector3(20.0f, 20.0f, 20.0f));
		s->SetBoundingRadius(100.0f);
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

	glGenFramebuffers(1, &bufferFBO); 
	glGenFramebuffers(1, &processFBO); 
	
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[0], 0); 
	
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
	GL_FRAMEBUFFER_COMPLETE || !bufferDepthTex || !bufferColourTex[0])
	{
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
	skyboxRotate = 0.0f;
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
	skyboxRotate += ROTATE_SPEED * dt;
	frameFrustum.FromMatrix(projMatrix * viewMatrix);

	frameTime -= dt;
	while (frameTime < 0.0f) {
		currentFrame = (currentFrame + 1) % charAnim->GetFrameCount();
		frameTime += 1.0f / charAnim->GetFrameRate();
	}
	root->Update(dt);
}


void Renderer::RenderScene() 
{
		DrawScene();
}

void Renderer::DrawScene()
{
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
	BindShader(sceneShader);
	UpdateShaderMatrices();
	
	DrawSkybox();
	DrawHeightmap();
	DrawCharAnim();
	BuildNodeLists(root);
	SortNodeLists();
	DrawNodes();
	ClearNodeLists();
	DrawWater();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D, bufferColourTex[1], 0);
		glUniform1i(glGetUniformLocation(processShader->GetProgram(), "isVertical"), 0);

		glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
		quad->Draw();
		// Now to swap the colour buffers , and do the second blur pass
		glUniform1i(glGetUniformLocation(processShader -> GetProgram(), "isVertical"), 1);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D, bufferColourTex[0], 0);
		glBindTexture(GL_TEXTURE_2D, bufferColourTex[1]);
		quad->Draw();

	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
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
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);

	quad->Draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void Renderer::DrawCharAnim()
{
	BindShader(charShader);
	glUniform1i(glGetUniformLocation(charShader->GetProgram(), "diffuseTex"), 0);
	MeshShaderMatrices();

	vector<Matrix4> frameMatrices;
	const Matrix4* invBindPose = charMesh->GetInverseBindPose();
	const Matrix4* frameData = charAnim->GetJointData(currentFrame);
	Vector3 hSize = heightMap->GetHeightmapSize();
	Matrix4 model = Matrix4::Translation(Vector3(int(hSize.x * 0.5f), 200.0f, int(hSize.z * 0.5f))) * Matrix4::Scale(Vector3(50.0f, 50.0f, 50.0f));

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
		
		glUniform4fv(glGetUniformLocation(treeShader->GetProgram(), "fogColour"), 1, (float*)fogColour);

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
	//if (frameFrustum.InsideFrustum(*from)) {
		Vector3 dir = from->GetWorldTransform().GetPositionVector() - camera->GetPosition();
		from->SetCameraDistance(Vector3::Dot(dir, dir));

		if (from->GetColour().w < 1.0f) {
			transparentNodeList.push_back(from);
		}
		else 
		{
			nodeList.push_back(from);
		}
	//}
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

	glUniform4fv(glGetUniformLocation(skyboxShader->GetProgram(), "fogColour"), 1, (float*)fogColour);

	viewMatrix = camera->BuildViewMatrix() * Matrix4::Rotation(skyboxRotate, Vector3(0, 1, 0));

	//textureMatrix = Matrix4::Rotation(skyboxRotate, Vector3(0, 1, 0));
	
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	
	UpdateShaderMatrices();
	quad->Draw();
	glDepthMask(GL_TRUE);
}

void Renderer::DrawHeightmap() {
	
	BindShader(lightShader);

	
	glUniform1i(glGetUniformLocation(lightShader -> GetProgram(), "ground"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, earthTex);

	glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "grass"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, grassTex);

	glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "rocks"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, rockTex);

	glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "rockBump"), 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, rockBump);

	glUniform3fv(glGetUniformLocation(lightShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glUniform4fv(glGetUniformLocation(treeShader->GetProgram(), "fogColour"), 1, (float*)fogColour);

	Vector3 hSize = heightMap->GetHeightmapSize();
	glUniform1f(glGetUniformLocation(lightShader->GetProgram(), "vHeight"), hSize.y);

	modelMatrix.ToIdentity();
	textureMatrix.ToIdentity();

	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	UpdateShaderMatrices();
	SetShaderLight(*light);
	heightMap -> Draw();
	
}

void Renderer::DrawWater() {
	BindShader(reflectShader);

	glUniform3fv(glGetUniformLocation(reflectShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "cubeTex"), 2);

	glUniform4fv(glGetUniformLocation(reflectShader->GetProgram(), "fogColour"), 1, (float*)fogColour);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, waterTex);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	Vector3 hSize = heightMap -> GetHeightmapSize();
	modelMatrix = Matrix4::Translation(Vector3(hSize.x * 0.5f, 30.0f, hSize.z *0.5f)) * Matrix4::Scale(hSize * 0.5f) * Matrix4::Rotation(90, Vector3(1, 0, 0));
	textureMatrix = Matrix4::Translation(Vector3(waterCycle, 0.0f, waterCycle)) * Matrix4::Scale(Vector3(10, 10, 10)) * Matrix4::Rotation(waterRotate, Vector3(0, 0, 1));
	
	textureMatrix.ToIdentity();
	UpdateShaderMatrices();
	quad -> Draw();
}