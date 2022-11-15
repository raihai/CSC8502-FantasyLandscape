#pragma once
#include "..\nclgl\SceneNode.h"

class Tree : public SceneNode {
public:
	 Tree(Mesh * tree);
	~Tree(void) {};
	void Update(float dt) override;

protected:
	SceneNode * head;
};
