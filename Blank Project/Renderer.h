# pragma once
#include "../nclgl/OGLRenderer.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/Frustum.h"


class Camera;
class Shader;
class HeightMap;
class Mesh;
class MeshMaterial;
class MeshAnimation;

class Renderer : public OGLRenderer {
public:
	Renderer(Window & parent);
	~Renderer(void);
	void RenderScene() override;
	void UpdateScene(float dt) override;
	void PresentScene();
	void DrawPostProcess();

protected:
	void DrawScene();

	void DrawHeightmap();
	void DrawWater();
	void DrawSkybox();
	
	void DrawCharAnim();
	void DrawNodes();
	void DrawNode(SceneNode* n);

	void BuildNodeLists(SceneNode* from);
	void SortNodeLists();
	void ClearNodeLists();


	
	Shader * lightShader;
	Shader * reflectShader;
	Shader * skyboxShader;
	Shader* treeShader;
	Shader* charShader;

	Shader* sceneShader;
	Shader * processShader;

	
	HeightMap * heightMap;
	Mesh * quad;
	Mesh* tree;
	Mesh* charMesh;
	
	MeshAnimation* charAnim;
	MeshMaterial* charMat;
	vector <GLuint> charTextures;

	MeshMaterial* material;
	vector <GLuint> matTextures;
	
	SceneNode* root;

	Light * light;
	Camera * camera;
	int currentFrame;
	float frameTime;

	Frustum frameFrustum;

	GLuint cubeMap;
	GLuint waterTex;
	GLuint earthTex;
	GLuint grassTex;
	GLuint rockTex;
	GLuint rockBump;

	GLuint bufferFBO;
	GLuint processFBO;
	GLuint bufferColourTex[2];
	GLuint bufferDepthTex;

	float sceneTime;
	float waterRotate;
	float waterCycle;

	vector <SceneNode*> transparentNodeList;
	vector <SceneNode*> nodeList;
	
	
};

