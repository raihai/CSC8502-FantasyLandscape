#pragma once
#include <string>
#include "Mesh.h"

class HeightMap : public Mesh {
public:
	HeightMap(const std::string& name);
	~HeightMap(void) {};

	Vector3 GetHeightmapSize() const { return heightmapSize; }

	Vector4 calculateColours(float a);

protected:
	Vector3 heightmapSize;
};