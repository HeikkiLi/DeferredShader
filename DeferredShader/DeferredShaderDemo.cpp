//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Deferred Shader demo project, a simple Directx11 application implementing deferred shading with
// different light types.
// 
// Features:
// - deferred shading
// - Gbuffer, different lights: directional, spot, point lights
// - Percentage-closer filtering (pcf) shadows for point and spot light
// - cascaded shadow map directional light
// - Hemispheric ambient color (will later replace with SSAO or in next project)
// - win32 window management and message handler
// - Mesh, ObjLoader, Material, GeometryGenerator (basic meshes grid, box etc)
// - simple camera implementation
// - GUI settings with Dear ImGui.
//
// 3rd Party libraries:
// Dear ImGui 
// https://github.com/ocornut/imgui
// 
// tiny obj loader 
// https://github.com/syoyo/tinyobjloader
//
// DirectXTex
// https://github.com/Microsoft/DirectXTex
// DirectXTex ScreenCrab 
// DirectXTex WICTextureLoader
// 
// DirectXTK
// In some parts using the SimpleMath helpers to wrap DirectXMath SIMD vector/matrix math API
// https://github.com/microsoft/DirectXTK
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include "Renderer/D3DRendererApp.h"
#include "Renderer/Camera.h"
#include "Renderer/GBuffer.h"
#include "Renderer/SceneManager.h"
#include "Renderer/LightManager.h"
#include "Renderer/Util.h"

enum RENDER_STATE { BACKBUFFERRT, DEPTHRT, COLSPECRT, NORMALRT, SPECPOWRT };

class DeferredShaderApp : public D3DRendererApp
{
public:
	DeferredShaderApp(HINSTANCE hInstance);
	~DeferredShaderApp();

	bool Init() override;
	void OnResize() override;
	void Update(float dt)override;
	void Render() override;

	void OnMouseDown(WPARAM btnState, int x, int y) override;
	void OnMouseUp(WPARAM btnState, int x, int y) override;
	void OnMouseMove(WPARAM btnState, int x, int y) override;

private:
	POINT mLastMousePos;

	Camera* mCamera;

	// D3D resources
	ID3D11SamplerState*	mSampPoint = NULL;
	ID3D11SamplerState*	mSampLinear = NULL;
	ID3D11VertexShader*	mGBufferVisVertexShader = NULL;
	ID3D11PixelShader*	mGBufferVisPixelShader = NULL;

	ID3D11VertexShader* mTextureVisVS = NULL;
	ID3D11PixelShader* mTextureVisDepthPS = NULL;
	ID3D11PixelShader* mTextureVisCSpecPS = NULL;
	ID3D11PixelShader* mTextureVisNormalPS = NULL;
	ID3D11PixelShader* mTextureVisSpecPowPS = NULL;

	// for managing the scene
	SceneManager mSceneManager;
	LightManager mLightManager;

	// GBuffer
	GBuffer mGBuffer;
	bool mVisualizeGBuffer;
	void VisualizeGBuffer();
	void VisualizeFullScreenGBufferTexture();

	// Light values
	bool mVisualizeLightVolume;
	XMVECTOR mAmbientLowerColor;
	XMVECTOR mAmbientUpperColor;
	XMVECTOR mDirLightDir;
	XMVECTOR mDirLightColor;

	bool mDirCastShadows;
	bool mAntiFlickerOn;
	bool mVisualizeCascades;

	XMFLOAT3	mSpotDirection;
	float		mSpotRange;
	float		mSpotOuterAngle; 
	float		mSpotInnerAngle;
	bool		mSpotCastShadows;

	bool		mPointCastShadows;

	int mNumLights;
	std::vector<XMFLOAT3> mLightPositions;
	float mLightRange;
	std::vector<XMFLOAT3> mLightColor;

	int mLightType;

	void RenderGUI();
	bool mShowSettings;
	bool mShowShadowMap;

	RENDER_STATE mRenderState;
};



// The Application entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	DeferredShaderApp shaderApp(hInstance);

	if (!shaderApp.Init())
		return 0;

	return shaderApp.Run();
}

DeferredShaderApp::DeferredShaderApp(HINSTANCE hInstance)
	: D3DRendererApp(hInstance)
{
	
	mMainWndCaption = L"DeferredShader Demo";
	mCamera = NULL;
	mVisualizeGBuffer = false;
	mShowSettings = true;
	mShowShadowMap = false;
	mLightType = 0;
	
	mVisualizeLightVolume = false;
	mNumLights = 3;
	mLightRange = 25.0f;

	mLightPositions.push_back( XMFLOAT3(3.0f, 5.0f, 5.0f) );
	mLightPositions.push_back( XMFLOAT3(-10.0f, 4.0f, -5.0f) );
	mLightPositions.push_back( XMFLOAT3(-5.0f, 10.0f, -5.0f));
	mLightColor.push_back( XMFLOAT3(0.0f, 1.0f, 0.0f) );
	mLightColor.push_back( XMFLOAT3(0.0f, 1.0f, 0.0f) );
	mLightColor.push_back(XMFLOAT3(1.0f, 1.0f, 1.0f) );

	XMFLOAT3 spotLook = XMFLOAT3(0, 0, 0);
	XMVECTOR dir = (XMLoadFloat3(&spotLook) - XMLoadFloat3(&mLightPositions[2]));
	dir = XMVector3Normalize(dir);
	XMStoreFloat3(&mSpotDirection, dir);
	mSpotRange = 30.0f;
	mSpotOuterAngle = 20.0f;
	mSpotInnerAngle = 15.0f;
	mSpotCastShadows = true;

	mPointCastShadows = false;

	// init light values
	mAmbientLowerColor = XMVectorSet(0.1f, 0.2f, 0.1f, 1.0f);
	mAmbientUpperColor = XMVectorSet(0.1f, 0.2f, 0.2f, 1.0f);
	mDirLightDir = XMVectorSet(-0.1, -0.4f, -0.9f, 1.0f);
	mDirLightColor = XMVectorSet(0.8f, 0.8f, 0.8f, 1.0f);
	mDirCastShadows = true;
	mAntiFlickerOn = true;
	mVisualizeCascades = false;

	mRenderState = RENDER_STATE::BACKBUFFERRT;
}

DeferredShaderApp::~DeferredShaderApp()
{
	SAFE_RELEASE(mSampLinear);
	SAFE_RELEASE(mSampPoint);
	SAFE_RELEASE(mGBufferVisVertexShader);
	SAFE_RELEASE(mGBufferVisPixelShader);

	SAFE_RELEASE(mTextureVisVS);

	SAFE_DELETE(mCamera);

	SAFE_RELEASE(mTextureVisDepthPS);
	SAFE_RELEASE(mTextureVisCSpecPS);
	SAFE_RELEASE(mTextureVisNormalPS);
	SAFE_RELEASE(mTextureVisSpecPowPS);

	mSceneManager.Release();
	mLightManager.Release();
	mGBuffer.Release();
}

bool DeferredShaderApp::Init()
{
	mCamera = new Camera();

	mClientWidth = 1024;
	mClientHeight = 768;

	if (!D3DRendererApp::Init())
		return false;

	// Shader for visualizing GBuffer
	HRESULT hr;
	WCHAR str[MAX_PATH] = L"..\\DeferredShader\\Shaders\\GBufferVisualize.hlsl";

	// compile shaders
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

	ID3DBlob* pShaderBlob = NULL;
	if (!CompileShader(str, NULL, "GBufferVisVS", "vs_5_0", dwShaderFlags, &pShaderBlob))
	{
		MessageBox(0, L"CompileShader Failed.", 0, 0);
		return false;
	}

	hr = md3dDevice->CreateVertexShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &mGBufferVisVertexShader);
	SAFE_RELEASE(pShaderBlob);
	if (FAILED(hr))
		return false;

	if (!CompileShader(str, NULL, "TextureVisVS", "vs_5_0", dwShaderFlags, &pShaderBlob))
	{
		MessageBox(0, L"CompileShader Failed.", 0, 0);
		return false;
	}

	hr = md3dDevice->CreateVertexShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &mTextureVisVS);
	SAFE_RELEASE(pShaderBlob);
	if (FAILED(hr))
		return false;

	if (!CompileShader(str, NULL, "GBufferVisPS", "ps_5_0", dwShaderFlags, &pShaderBlob))
	{
		MessageBox(0, L"CompileShader Failed.", 0, 0);
		return false;
	}
	hr = md3dDevice->CreatePixelShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &mGBufferVisPixelShader);
	SAFE_RELEASE(pShaderBlob);
	if (FAILED(hr))
		return false;

	if (!CompileShader(str, NULL, "TextureVisDepthPS", "ps_5_0", dwShaderFlags, &pShaderBlob))
	{
		MessageBox(0, L"CompileShader Failed.", 0, 0);
		return false;
	}
	hr = md3dDevice->CreatePixelShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &mTextureVisDepthPS);
	SAFE_RELEASE(pShaderBlob);
	if (FAILED(hr))
		return false;
	

	if (!CompileShader(str, NULL, "TextureVisCSpecPS", "ps_5_0", dwShaderFlags, &pShaderBlob))
	{
		MessageBox(0, L"CompileShader Failed.", 0, 0);
		return false;
	}
	hr = md3dDevice->CreatePixelShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &mTextureVisCSpecPS);
	SAFE_RELEASE(pShaderBlob);
	if (FAILED(hr))
		return false;

	if (!CompileShader(str, NULL, "TextureVisNormalPS", "ps_5_0", dwShaderFlags, &pShaderBlob))
	{
		MessageBox(0, L"CompileShader Failed.", 0, 0);
		return false;
	}
	hr = md3dDevice->CreatePixelShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &mTextureVisNormalPS);
	SAFE_RELEASE(pShaderBlob);
	if (FAILED(hr))
		return false;

	if (!CompileShader(str, NULL, "TextureVisSpecPowPS", "ps_5_0", dwShaderFlags, &pShaderBlob))
	{
		MessageBox(0, L"CompileShader Failed.", 0, 0);
		return false;
	}
	hr = md3dDevice->CreatePixelShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &mTextureVisSpecPowPS);
	SAFE_RELEASE(pShaderBlob);
	if (FAILED(hr))
		return false;


	// create samplers
	D3D11_SAMPLER_DESC samDesc;
	ZeroMemory(&samDesc, sizeof(samDesc));
	samDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samDesc.MaxAnisotropy = 1;
	samDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samDesc.MaxLOD = D3D11_FLOAT32_MAX;
	if (FAILED(md3dDevice->CreateSamplerState(&samDesc, &mSampLinear)))
	{
		return false;
	}

	samDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	if (FAILED(md3dDevice->CreateSamplerState(&samDesc, &mSampPoint)))
	{
		return false;
	}

	// init camera
	mCamera->LookAt(XMFLOAT3(12.0, 6.0, -15.0), XMFLOAT3(-0.6, -0.2, 0.8), XMFLOAT3(0.0, 1.0, 0.0));
	mCamera->SetLens(0.25f*M_PI, AspectRatio(), 1.0f, 1000.0f);
	mCamera->UpdateViewMatrix();

	if (!mSceneManager.Init(md3dDevice, mCamera))
		return false;

	V_RETURN(mLightManager.Init(md3dDevice, mCamera));

	return true;
}

void DeferredShaderApp::OnResize()
{
	D3DRendererApp::OnResize();

	// Recreate the GBuffer with the new size
	mGBuffer.Init(md3dDevice, mClientWidth, mClientHeight);

	// update camera
	mCamera->SetLens(0.25f*M_PI, AspectRatio(), 1.0f, 1000.0f);
	mCamera->UpdateViewMatrix();


}

void DeferredShaderApp::Update(float dt)
{
	// set ambient colors
	mLightManager.SetAmbient(mAmbientLowerColor, mAmbientUpperColor);

	///// sun / directional light
	mLightManager.SetDirectional(mDirLightDir, mDirLightColor, mDirCastShadows, mAntiFlickerOn);

	mCamera->UpdateViewMatrix();

	if (GetAsyncKeyState(VK_F2) & 0x01)
		mVisualizeGBuffer = !mVisualizeGBuffer;

	if (GetAsyncKeyState(VK_F3) & 0x01)
		mShowShadowMap = !mShowShadowMap;

	if (GetAsyncKeyState(VK_F4) & 0x01)
	{
		// Save backbuffer
		LPCTSTR screenshotFileName = L"screenshot.jpg";
		SnapScreenshot(screenshotFileName);
	}

	if (GetAsyncKeyState(VK_F11) & 0x01)
		mShowSettings = !mShowSettings;

	if (GetAsyncKeyState(VK_DOWN) & 0x01)
		mCamera->Walk(-dt*50.0f);

	if (GetAsyncKeyState(VK_UP) & 0x01)
		mCamera->Walk(dt * 50.0f);

	if (GetAsyncKeyState(0x31) & 0x01)
		mRenderState = RENDER_STATE::BACKBUFFERRT;
	if (GetAsyncKeyState(0x32) & 0x01)
		mRenderState = RENDER_STATE::DEPTHRT;
	if (GetAsyncKeyState(0x33) & 0x01)
		mRenderState = RENDER_STATE::COLSPECRT;
	if (GetAsyncKeyState(0x34) & 0x01)
		mRenderState = RENDER_STATE::NORMALRT;
	if (GetAsyncKeyState(0x35) & 0x01)
		mRenderState = RENDER_STATE::SPECPOWRT;

	/////  Rest of the lights
	mLightManager.ClearLights();
	
	// point lights
	//mLightManager.AddPointLight(mLightPositions[1], mLightRange, mLightColor[1], mPointCastShadows);
	
	// spot light
	//mLightManager.AddSpotLight(mLightPositions[2], mSpotDirection, mSpotRange, mSpotOuterAngle, mSpotInnerAngle, mLightColor[2], mSpotCastShadows);

}

void DeferredShaderApp::Render()
{
	
	// Store the current states
	D3D11_VIEWPORT oldvp;
	UINT num = 1;
	md3dImmediateContext->RSGetViewports(&num, &oldvp);
	ID3D11RasterizerState* pPrevRSState;
	md3dImmediateContext->RSGetState(&pPrevRSState);

	// Generate the shadow maps
	while (mLightManager.PrepareNextShadowLight(md3dImmediateContext))
	{
		mSceneManager.RenderSceneNoShaders(md3dImmediateContext);
	}

	// Restore the states
	md3dImmediateContext->RSSetViewports(num, &oldvp);
	md3dImmediateContext->RSSetState(pPrevRSState);
	SAFE_RELEASE(pPrevRSState);
	// Cleanup
	md3dImmediateContext->VSSetShader(NULL, NULL, 0);
	md3dImmediateContext->GSSetShader(NULL, NULL, 0);
	

	// clear render target view
	float clearColor[4] = { 0.4f, 0.4, 0.8, 0.0f };
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, clearColor);

	// clear depth stencil view
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH, 1.0, 0);

	// world, projection, view matrix
	XMMATRIX world = XMMatrixIdentity();
	XMMATRIX worldViewProj = mCamera->ViewProj();

	// Store depth state
	ID3D11DepthStencilState* pPrevDepthState;
	UINT nPrevStencil;
	md3dImmediateContext->OMGetDepthStencilState(&pPrevDepthState, &nPrevStencil);

	// set samplers
	ID3D11SamplerState* samplers[2] = { mSampLinear, mSampPoint };
	md3dImmediateContext->PSSetSamplers(0, 2, samplers);

	// Render to GBuffer
	mGBuffer.PreRender(md3dImmediateContext);
	mSceneManager.Render(md3dImmediateContext);
	mGBuffer.PostRender(md3dImmediateContext);

	// set render target
	md3dImmediateContext->OMSetRenderTargets(1, &mRenderTargetView, mGBuffer.GetDepthReadOnlyDSV());
	mGBuffer.PrepareForUnpack(md3dImmediateContext, mCamera);
	
	// do lighting
	mLightManager.DoLighting(md3dImmediateContext, &mGBuffer, mCamera);

	// Add the light sources wireframe on top of the LDR target
	if (mVisualizeLightVolume)
	{
		mLightManager.DoDebugLightVolume(md3dImmediateContext, mCamera);
	}

	// show debug buffers
	if (mVisualizeGBuffer)
	{
		md3dImmediateContext->OMSetRenderTargets(1, &mRenderTargetView, NULL);

		VisualizeGBuffer();

		md3dImmediateContext->OMSetRenderTargets(1, &mRenderTargetView, mGBuffer.GetDepthDSV());

	}

	
	if (mRenderState != RENDER_STATE::BACKBUFFERRT)
	{
		md3dImmediateContext->OMSetRenderTargets(1, &mRenderTargetView, NULL);

		VisualizeFullScreenGBufferTexture();

		md3dImmediateContext->OMSetRenderTargets(1, &mRenderTargetView, mGBuffer.GetDepthDSV());
	}

	// Show the shadow cascades
	if (mVisualizeCascades && mDirCastShadows)
	{
		mLightManager.DoDebugCascadedShadows(md3dImmediateContext, &mGBuffer);
	}

	// Show shadowmap
	if (mShowShadowMap)
	{
		mLightManager.VisualizeShadowMap(md3dImmediateContext);
	}

	// retore prev depth state
	md3dImmediateContext->OMSetDepthStencilState(pPrevDepthState, nPrevStencil);
	SAFE_RELEASE(pPrevDepthState);

	// Render gui
	RenderGUI();
	

	md3dImmediateContext->RSSetViewports(1, &mScreenViewport);
	HR(mSwapChain->Present(0, 0));
}

void DeferredShaderApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void DeferredShaderApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void DeferredShaderApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_RBUTTON) != 0)
	{
		// Each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		// move Camera
		mCamera->Pitch(dy);
		mCamera->RotateY(dx);
	}
	else if ((btnState & MK_MBUTTON) != 0 && ( x != mLastMousePos.x && y != mLastMousePos.y) )
	{
		// if middle mouse button then rotate directional light.
		float fDX = (float)(x - mLastMousePos.x) * 0.02f;
		float fDY = (float)(y - mLastMousePos.y) * 0.02f;
		
		XMVECTOR pDeterminant;
		XMFLOAT4X4 matViewInv;
		XMStoreFloat4x4(&matViewInv, XMMatrixInverse(&pDeterminant, mCamera->View()));

		XMVECTOR right = XMLoadFloat3( &XMFLOAT3(matViewInv._11, matViewInv._12, matViewInv._13));
		XMVECTOR up = XMLoadFloat3( &XMFLOAT3(matViewInv._21, matViewInv._22, matViewInv._23));
		XMVECTOR forward = XMLoadFloat3( &XMFLOAT3(matViewInv._31, matViewInv._32, matViewInv._33));

		mDirLightDir -= right * fDX;
		mDirLightDir -= up * fDY;
		mDirLightDir += forward * fDY;

		mDirLightDir = XMVector3Normalize(mDirLightDir);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void DeferredShaderApp::VisualizeGBuffer()
{
	ID3D11ShaderResourceView* arrViews[4] = { mGBuffer.GetDepthView(), mGBuffer.GetColorView(), mGBuffer.GetNormalView() , mGBuffer.GetSpecPowerView() };
	md3dImmediateContext->PSSetShaderResources(0, 4, arrViews);

	md3dImmediateContext->PSSetSamplers(1, 1, &mSampPoint);

	md3dImmediateContext->IASetInputLayout(NULL);
	md3dImmediateContext->IASetVertexBuffers(0, 0, NULL, NULL, NULL);
	md3dImmediateContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// Set the shaders
	md3dImmediateContext->VSSetShader(mGBufferVisVertexShader, NULL, 0);
	md3dImmediateContext->GSSetShader(NULL, NULL, 0);
	md3dImmediateContext->PSSetShader(mGBufferVisPixelShader, NULL, 0);

	md3dImmediateContext->Draw(16, 0);

	// Cleanup
	md3dImmediateContext->VSSetShader(NULL, NULL, 0);
	md3dImmediateContext->PSSetShader(NULL, NULL, 0);

	ZeroMemory(arrViews, sizeof(arrViews));
	md3dImmediateContext->PSSetShaderResources(0, 4, arrViews);
}

void DeferredShaderApp::VisualizeFullScreenGBufferTexture()
{
	ID3D11ShaderResourceView* arrViews[4] = { mGBuffer.GetDepthView(), mGBuffer.GetColorView(), mGBuffer.GetNormalView() , mGBuffer.GetSpecPowerView() };
	md3dImmediateContext->PSSetShaderResources(0, 4, arrViews);

	md3dImmediateContext->PSSetSamplers(1, 1, &mSampPoint);

	md3dImmediateContext->IASetInputLayout(NULL);
	md3dImmediateContext->IASetVertexBuffers(0, 0, NULL, NULL, NULL);
	md3dImmediateContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// Set the shaders
	md3dImmediateContext->VSSetShader(mTextureVisVS, NULL, 0);
	md3dImmediateContext->GSSetShader(NULL, NULL, 0);
	switch (mRenderState)
	{
	case DEPTHRT:
		md3dImmediateContext->PSSetShader(mTextureVisDepthPS, NULL, 0);
		break;
	case COLSPECRT:
		md3dImmediateContext->PSSetShader(mTextureVisCSpecPS, NULL, 0);
		break;
	case NORMALRT:
		md3dImmediateContext->PSSetShader(mTextureVisNormalPS, NULL, 0);
		break;
	case SPECPOWRT:
		md3dImmediateContext->PSSetShader(mTextureVisSpecPowPS, NULL, 0);
		break;
	default:
		break;
	}
	

	md3dImmediateContext->Draw(4, 0);

	// Cleanup
	md3dImmediateContext->VSSetShader(NULL, NULL, 0);
	md3dImmediateContext->PSSetShader(NULL, NULL, 0);

	ZeroMemory(arrViews, sizeof(arrViews));
	md3dImmediateContext->PSSetShaderResources(0, 4, arrViews);
}

void DeferredShaderApp::RenderGUI()
{
	ImGui_ImplDX11_NewFrame();
	{ //using brackets to control scope makes formatting and checking where the ImGui::Render(); is easier.

		if (mShowRenderStats)
		{
			ImGui::Begin("Framerate", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
			ImGui::SetWindowSize(ImVec2(200, 30), ImGuiSetCond_FirstUseEver);
			ImGui::SetWindowPos(ImVec2(2, 2), ImGuiSetCond_FirstUseEver);
			ImGui::Text("%.3f ms/frame (%.1f FPS)", mFrameStats.mspf, mFrameStats.fps);
			ImGui::End();
		}

		if (mShowSettings)
		{
			ImGui::Begin("Settings", 0, 0);
			ImGui::SetWindowSize(ImVec2(200, 600), ImGuiSetCond_FirstUseEver);
			ImGui::SetWindowPos(ImVec2(10, 60), ImGuiSetCond_FirstUseEver);

			ImGui::Spacing();
			if (ImGui::CollapsingHeader("Lights"))
			{
				if (ImGui::CollapsingHeader("Directional"))
				{
					ImGui::Text("Color:");
					static ImVec4 color = ImColor(mDirLightColor.m128_f32[0], mDirLightColor.m128_f32[1], mDirLightColor.m128_f32[2]);
					ImGui::ColorEdit3("DirLightColor##dcol1", (float*)&color, ImGuiColorEditFlags_NoLabel);

					mDirLightColor = XMLoadFloat3(&XMFLOAT3((float*)&color));

					ImGui::Checkbox("Shadows##dirshadow", &mDirCastShadows); 
					ImGui::Checkbox("Visualize Cascades##vcascades", &mVisualizeCascades);
					ImGui::Checkbox("Antiflicker", &mAntiFlickerOn);
				}

				// Spot light settings
				/*
				if (ImGui::CollapsingHeader("Spot"))
				{
					
					static float spotDir[3]  = { mSpotDirection.x, mSpotDirection.y, mSpotDirection.z };
					ImGui::SliderFloat3("direction", spotDir, -1.0f, 1.0f);
					mSpotDirection = XMFLOAT3((float*)&spotDir);

					ImGui::SliderFloat("range", &mSpotRange, 0.1f, 100.0f, "%.3f");
					ImGui::SliderFloat("outer angle", &mSpotOuterAngle, 0.0f, 100.0f, "%.3f");
					ImGui::SliderFloat("inner angle", &mSpotInnerAngle, 0.0f, 100.0f, "%.3f");

					ImGui::Text("Color:");
					static ImVec4 color = ImColor(mLightColor[2].x, mLightColor[2].y, mLightColor[2].z);
					ImGui::ColorEdit3("SpotLightColor##scol1", (float*)&color, ImGuiColorEditFlags_NoLabel);
					mLightColor[2] = XMFLOAT3((float*)&color);
					ImGui::Checkbox("Shadows##spotshadow", &mSpotCastShadows);
				}
				*/

				if (ImGui::CollapsingHeader("Point"))
				{
					ImGui::SliderFloat("point range", &mLightRange, 0.1f, 100.0f, "%.3f");
					ImGui::Checkbox("Shadows##pointshadow1", &mPointCastShadows);
				}
			}

			if (ImGui::CollapsingHeader("Ambient Colors"))
			{
				ImGui::TextWrapped("Hemispheric ambient values for up and lower ambient color.\n\n");

				ImGui::Text("Upper color:");
				static ImVec4 colorAUp = ImColor(mAmbientUpperColor.m128_f32[0], mAmbientUpperColor.m128_f32[1], mAmbientUpperColor.m128_f32[2]);
				ImGui::ColorEdit3("AmbientUpperColor##1", (float*)&colorAUp, ImGuiColorEditFlags_NoLabel);

				mAmbientUpperColor = XMLoadFloat3(&XMFLOAT3((float*)&colorAUp));

				ImGui::Text("Lower color:");
				static ImVec4 colorALower = ImColor(mAmbientLowerColor.m128_f32[0], mAmbientLowerColor.m128_f32[1], mAmbientLowerColor.m128_f32[2]);
				ImGui::ColorEdit3("AmbientLowerColor##1", (float*)&colorALower, ImGuiColorEditFlags_NoLabel);

				mAmbientLowerColor = XMLoadFloat3(&XMFLOAT3((float*)&colorALower));
			}
			if (ImGui::CollapsingHeader("Camera"))
			{
				float campos[3] = { mCamera->GetPosition().x, mCamera->GetPosition().y, mCamera->GetPosition().z };
				ImGui::SliderFloat3("position##campos", campos, -50.0f, 50.0f);
				mCamera->SetPosition(XMFLOAT3((float*)& campos));

			}

			ImGui::Checkbox("FrameStats (F1)", &mShowRenderStats);
			ImGui::Checkbox("Visualize Buffers (F2)", &mVisualizeGBuffer);
			ImGui::Checkbox("Visualize ShadowMap (F3)", &mShowShadowMap);
			ImGui::Checkbox("Visualize Light Volume", &mVisualizeLightVolume);
			ImGui::TextWrapped("\nToggle settings window (F11)");
			ImGui::TextWrapped("\nSave screenshot (F4).\n\n");

			ImGui::End();
		}

	}
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());


}