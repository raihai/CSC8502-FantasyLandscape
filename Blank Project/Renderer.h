# pragma once
#include "../nclgl/OGLRenderer.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/Frustum.h"
#include "Tree.h";

class Camera;
class Shader;
class HeightMap;
class Mesh;
class MeshMaterial;

class Renderer : public OGLRenderer {
public:
	Renderer(Window & parent);
	~Renderer(void);
	void RenderScene() override;
	void UpdateScene(float dt) override;
	
protected:
	void DrawHeightmap();
	void DrawWater();
	void DrawSkybox();
	
	void DrawNodes();
	void DrawNode(SceneNode* n);

	void BuildNodeLists(SceneNode* from);
	void SortNodeLists();
	void ClearNodeLists();

	
	Shader * lightShader;
	Shader * reflectShader;
	Shader * skyboxShader;
	Shader* shader;
	
	HeightMap * heightMap;
	Mesh * quad;
	Mesh* tree;

	MeshMaterial* material;
	vector <GLuint> matTextures;

	SceneNode* root;

	Light * light;
	Camera * camera;
	Frustum frameFrustum;

	GLuint cubeMap;
	GLuint waterTex;
	GLuint earthTex;
	GLuint earthBump;

	float sceneTime;
	float waterRotate;
	float waterCycle;

	vector <SceneNode*> transparentNodeList;
	vector <SceneNode*> nodeList;
	
	
};

