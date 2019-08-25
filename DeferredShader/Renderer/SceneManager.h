#pragma once

#include "Camera.h"
#include "Mesh.h"
#include "Util.h"

// SceneManager class
// Simple scenemanager that holds Scenes meshes and camera
// Just for testing simple scene this holds hardcoded
class SceneManager
{
public:

	SceneManager();
	~SceneManager();

	bool Init(ID3D11Device* device, Camera* camera);
	void Release();

	// Renders the scene meshes into the GBuffer
	void Render(ID3D11DeviceContext* pd3dImmediateContext);

	// Renders the scene with no shaders
	void RenderSceneNoShaders(ID3D11DeviceContext* pd3dImmediateContext);

private:

	// Scene meshes
	std::vector<Mesh*> mMeshes;

	// Scene meshes shader constant buffers
	ID3D11Buffer* mSceneVertexShaderCB;
	ID3D11Buffer* mScenePixelShaderCB;

	// Depth prepass vertex shader
	ID3D11VertexShader* mSceneVertexShader;
	ID3D11InputLayout* mSceneVSLayout;
	ID3D11PixelShader* mScenePixelShader;

	Camera* mCamera;
};