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

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	quad = Mesh::GenerateQuad();
	tree = Mesh::LoadFromMeshFile("Tree10_3.msh");
	material = new MeshMaterial("Tree10_3.mat");

	charMesh = Mesh::LoadFromMeshFile("Rumba Dancing.msh");
	charAnim = new MeshAnimation("RumbaDance.anm");
	charMat = new MeshMaterial("Rumba Dancing.mat");

	heightMap = new HeightMap(TEXTUREDIR "noise.png");

	waterTex = SOIL_load_OGL_texture(TEXTUREDIR "water.TGA", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthTex = SOIL_load_OGL_texture(TEXTUREDIR "ground4_diffuse.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthBump = SOIL_load_OGL_texture(TEXTUREDIR "ground4_Normal.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
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
	charShader = new Shader("SkinningVertex.glsl", "SceneFragment.glsl");


	if (!reflectShader->LoadSuccess() ||
		!skyboxShader->LoadSuccess() ||
		!lightShader->LoadSuccess()|| !treeShader -> LoadSuccess() || !charShader -> LoadSuccess()){
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
	for (int i = 0; i < 5; ++i) {
		SceneNode* s = new SceneNode();
		s->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));

		s->SetTransform(Matrix4::Translation(Vector3(rand() % int(heightmapSize.x), 100.0f, rand() % int(heightmapSize.z))));
		s->SetModelScale(Vector3(10.0f, 10.0f, 10.0f));
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
		SetTextureRepeating(charTextures[i], true);
	}



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

	DrawSkybox();
	DrawHeightmap();
	DrawCharAnim();
	BuildNodeLists(root);
	SortNodeLists();
	BindShader(treeShader);
	UpdateShaderMatrices();
	DrawNodes();
	ClearNodeLists();

	DrawWater();
}


void Renderer::DrawCharAnim()
{
	BindShader(charShader);
	glUniform1i(glGetUniformLocation(charShader->GetProgram(), "diffuseTex"), 0);

	UpdateShaderMatrices();

	vector<Matrix4> frameMatrices;

	const Matrix4* invBindPose = charMesh->GetInverseBindPose();
	const Matrix4* frameData = charAnim->GetJointData(currentFrame);

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