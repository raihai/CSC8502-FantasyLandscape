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
	void PresentTwoScene();
	void DrawPostProcess();
	void DrawTwoScene();

	void moveCamera();
	void stopCamera(); 

protected:
	void DrawScene();
	void animateCam(float dt);

	void DrawHeightmap();
	void DrawWater();
	void DrawSkybox();

	void DrawHeightmap2();
	void DrawWater2();
	void DrawSkybox2();
	
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
	Mesh* rightQuad;
	Mesh* leftQuad;

	Mesh* tree;
	Mesh* charMesh;
	
	MeshAnimation* charAnim;
	MeshMaterial* charMat;
	vector <GLuint> charTextures;

	MeshMaterial* material;
	vector <GLuint> matTextures;
	
	SceneNode* root;

	Light* light;
	Camera * camera;
	Camera* secCamera;
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
	GLuint secBufferFBO;

	GLuint bufferColourTex[3];
	GLuint bufferDepthTex;

	float sceneTime;
	float waterRotate;
	float waterCycle;
	float skyboxRotate;
	int stages = 0;
	float timeP = 0;

	bool moveCam;
	Vector3 firsPos;
	Vector3 secPos;
	Vector3 thirPos;
	Vector3 curPos;

	Vector4* fogColour;
	vector <SceneNode*> transparentNodeList;
	vector <SceneNode*> nodeList;
	
	
};

