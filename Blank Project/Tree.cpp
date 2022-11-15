#include "Tree.h"

Tree::Tree(Mesh * tree)
{
	SceneNode* body = new SceneNode(tree, Vector4(1, 0, 0, 1));
	body->SetModelScale(Vector3(30, 30, 30));
	body->SetTransform(Matrix4::Translation(Vector3(0, 35, 0)));
	body->SetBoundingRadius(1000.0f);
	AddChild(body);

	head = new  SceneNode(tree, Vector4(1, 0, 0, 1));

}

void Tree::Update(float dt)
{
	transform = transform * Matrix4::Rotation(30.0f * dt, Vector3(0, 1, 0));

	SceneNode::Update(dt);
}
