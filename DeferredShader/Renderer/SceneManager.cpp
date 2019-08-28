#include "SceneManager.h"
#include "LightManager.h"
#include "ObjLoader.h"
#include "GeometryGenerator.h"
#include "TextureManager.h"

#pragma pack(push,1)
struct CB_VS_PER_OBJECT
{
	XMMATRIX mWorldViewProjection;
	XMMATRIX mWorld;
};

struct CB_PS_PER_OBJECT
{
	XMFLOAT4 mdiffuseColor;
	float mSpecExp;
	float mSpecIntensity;
	bool mUseDiffuseTexture;
	bool mUseSpecularTexture;
	bool mUseNormalMapTexture;
	bool mUseAlphaTexture;
	float pad;
};
#pragma pack(pop)


SceneManager::SceneManager() : mSceneVertexShaderCB(NULL), mScenePixelShaderCB(NULL), mSceneVertexShader(NULL), mSceneVSLayout(NULL), mCamera(NULL),
mScenePixelShader(NULL)
{
}

SceneManager::~SceneManager()
{
	Release();
}

bool SceneManager::Init(ID3D11Device* device, Camera* camera)
{
	HRESULT hr;

	mMeshes.clear();

	// Load the models
	MeshData meshData;
	if (!ObjLoader::Instance()->LoadToMesh("..\\Assets\\bunny.obj",  "..\\Assets\\", meshData))
		return false;

	Mesh* mesh = new Mesh();
	mesh->Create(device, meshData);
	XMMATRIX matTranslate = XMMatrixTranslation(-1.0f, 0.0f, -1.0f);
	XMMATRIX matScale = XMMatrixScaling(4.0f, 4.0f, 4.0f);
	XMMATRIX matRot = XMMatrixRotationY(M_PI);
	mesh->mWorld = matTranslate * matScale * matRot; 
	mMeshes.push_back(mesh);
	
	MeshData meshDataTP;
	if (!ObjLoader::Instance()->LoadToMesh("..\\Assets\\teapot.obj", "..\\Assets\\", meshDataTP))
		return false;

	Mesh* mesh2 = new Mesh();
	mesh2->Create(device, meshDataTP);
	matTranslate = XMMatrixTranslation(4.0f, 0.0f, -2.0f);
	matScale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
	matRot = XMMatrixRotationY(0.4*M_PI);
	mesh2->mWorld = matTranslate * matScale * matRot;
	mMeshes.push_back(mesh2);
	
	// Grid 
	//GeometryGenerator::Instance()->CreateGrid(12.0f, 12.0f, 4, 4, meshData);
	MeshData meshData2;
	if (!ObjLoader::Instance()->LoadToMesh("..\\Assets\\cube\\cube.obj", "..\\Assets\\cube\\", meshData2))
				return false;
	//Material gridMat;
	//gridMat.Diffuse = XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f);
	//gridMat.specExp = 10.0f;
	//gridMat.specIntensivity = 1.0f;
	//meshData.materials[0] = gridMat;
	mesh = new Mesh();
	mesh->Create(device, meshData2);
	//mesh->mMaterials[0] = gridMat;
	matScale = XMMatrixScaling(20.0f, 0.1f, 20.0f);
	mesh->mWorld = XMMatrixIdentity() * matScale;
	mMeshes.push_back(mesh);

	// Create constant buffers
	D3D11_BUFFER_DESC cbDesc;
	ZeroMemory(&cbDesc, sizeof(cbDesc));
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.ByteWidth = sizeof(CB_VS_PER_OBJECT);
	if (FAILED(device->CreateBuffer(&cbDesc, NULL, &mSceneVertexShaderCB)))
		return false;

	cbDesc.ByteWidth = sizeof(CB_PS_PER_OBJECT);
	if (FAILED(device->CreateBuffer(&cbDesc, NULL, &mScenePixelShaderCB)))
		return false;

	// Read the HLSL file
	WCHAR str[MAX_PATH] = L"..\\DeferredShader\\Shaders\\DeferredShading.hlsl";

	// Compile the shaders
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

	// Load the prepass light shader
	ID3DBlob* pShaderBlob = NULL;
	if (FAILED(CompileShader(str, NULL, "RenderSceneVS", "vs_5_0", dwShaderFlags, &pShaderBlob)))
		return false;

	if (FAILED(device->CreateVertexShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &mSceneVertexShader)))
	{
		return false;
	}

	// Create a layout for the object data
	const D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	hr = device->CreateInputLayout(layout, ARRAYSIZE(layout), pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), &mSceneVSLayout);
	SAFE_RELEASE(pShaderBlob);
	if (FAILED(hr))
		return false;

	if (FAILED(CompileShader(str, NULL, "RenderScenePS", "ps_5_0", dwShaderFlags, &pShaderBlob)))
		return false;
	hr = device->CreatePixelShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &mScenePixelShader);
	SAFE_RELEASE(pShaderBlob);
	if (FAILED(hr))
		return false;

	mCamera = camera;

	return true;
}

void SceneManager::Release()
{
	if (mMeshes.size() > 0)
	{
		for (int i = 0; i < mMeshes.size(); ++i)
		{
			if (mMeshes[i] != NULL)
			{
				mMeshes[i]->Destroy();
				mMeshes[i] = NULL;
			}
		}
	}

	SAFE_RELEASE(mSceneVertexShaderCB);
	SAFE_RELEASE(mScenePixelShaderCB);
	SAFE_RELEASE(mSceneVertexShader);
	SAFE_RELEASE(mSceneVSLayout);
	SAFE_RELEASE(mScenePixelShader);
}


// Renders the scene to GBuffer
void SceneManager::Render(ID3D11DeviceContext* pd3dImmediateContext)
{
	// Get the projection & view matrix from the camera class
	
	XMMATRIX mView = mCamera->View();
	XMMATRIX mProj = mCamera->Proj();


	// Render the meshes
	for (int i = 0; i < mMeshes.size(); ++i)
	{
		// mesh world matrix
		XMMATRIX mWorld = mMeshes[i]->mWorld;
		XMMATRIX mWorldViewProjection = mWorld * mView * mProj;

		// Set the constant buffers
		HRESULT hr;
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		HR(pd3dImmediateContext->Map(mSceneVertexShaderCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
		CB_VS_PER_OBJECT* pVSPerObject = (CB_VS_PER_OBJECT*)MappedResource.pData;
		pVSPerObject->mWorldViewProjection = XMMatrixTranspose(mWorldViewProjection);
		pVSPerObject->mWorld = XMMatrixTranspose(mWorld);
		pd3dImmediateContext->Unmap(mSceneVertexShaderCB, 0);
		pd3dImmediateContext->VSSetConstantBuffers(0, 1, &mSceneVertexShaderCB);

		HR(pd3dImmediateContext->Map(mScenePixelShaderCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
		CB_PS_PER_OBJECT* pPSPerObject = (CB_PS_PER_OBJECT*)MappedResource.pData;
		//pPSPerObject->mEyePosition = mCamera->GetPosition();
		// set per object properties
		pPSPerObject->mSpecExp = mMeshes[i]->mMaterials[0].specExp;
		pPSPerObject->mSpecIntensity = mMeshes[i]->mMaterials[0].specIntensivity;
		pPSPerObject->mdiffuseColor = mMeshes[i]->mMaterials[0].Diffuse;
		
		if (TextureManager::Instance()->GetTexture(mMeshes[i]->mMaterials[0].diffuseTexture) != NULL)
		{
			 
			ID3D11ShaderResourceView* srv = TextureManager::Instance()->GetTexture(mMeshes[i]->mMaterials[0].diffuseTexture);
			pd3dImmediateContext->PSSetShaderResources(0, 1, &srv);

			
			pPSPerObject->mUseDiffuseTexture = true;
		}
		else {
			pPSPerObject->mUseDiffuseTexture = false;
		
		}

		pPSPerObject->mUseSpecularTexture = false;
		pPSPerObject->mUseNormalMapTexture = false;
		pPSPerObject->mUseAlphaTexture = false;

		pd3dImmediateContext->Unmap(mScenePixelShaderCB, 0);
		pd3dImmediateContext->PSSetConstantBuffers(0, 1, &mScenePixelShaderCB);

		// Set the vertex layout
		pd3dImmediateContext->IASetInputLayout(mSceneVSLayout);

		// Set the shaders
		pd3dImmediateContext->VSSetShader(mSceneVertexShader, NULL, 0);
		pd3dImmediateContext->PSSetShader(mScenePixelShader, NULL, 0);

		// render
		mMeshes[i]->Render(pd3dImmediateContext);
	}


}

void SceneManager::RenderSceneNoShaders(ID3D11DeviceContext * pd3dImmediateContext)
{

	XMMATRIX mView = mCamera->View();
	XMMATRIX mProj = mCamera->Proj();

	// render meshes
	for (int i = 0; i < mMeshes.size(); ++i)
	{
		// set object world matrix
		XMMATRIX mWorld = mMeshes[i]->mWorld;
		XMMATRIX mWorldViewProjection = mWorld * mView * mProj;

		// Set the constant buffers
		HRESULT hr;
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		HR(pd3dImmediateContext->Map(mSceneVertexShaderCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
		CB_VS_PER_OBJECT* pVSPerObject = (CB_VS_PER_OBJECT*)MappedResource.pData;
		pVSPerObject->mWorldViewProjection = XMMatrixTranspose(mWorldViewProjection);
		pVSPerObject->mWorld = XMMatrixTranspose(mWorld);
		pd3dImmediateContext->Unmap(mSceneVertexShaderCB, 0);
		pd3dImmediateContext->VSSetConstantBuffers(0, 1, &mSceneVertexShaderCB);

		// render mesh, sets vertex and index buffers
		mMeshes[i]->Render(pd3dImmediateContext);
	}

}
