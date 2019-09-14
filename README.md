# DeferredShader

![Deferred Shader with visualization of GBuffer.](https://i2.wp.com/heikkili.kapsi.fi/blog/wp-content/uploads/2019/09/screenshot_1.jpg)

Deferred Shader demo project, a simple DirectX11 application implementing deferred shading with different light types.


## Controls:

Keys:
- F1 Framestats
- F2 Visualize GBuffer
- F3 Visualize ShadowMap
- F4 Save screenshot
- F11 Toggle settings window
- ESC Quit

Mouse:
- MMB Rotate directional light
- RMB Rotate camera look 

## Features:
 
- deferred shading
- GBuffer, different lights: directional, spot, point lights
- Percentage-closer filtering (pcf) shadows for point and spot light
- cascaded shadow map directional light
- Hemispheric ambient color (will later replace with SSAO or in next project)
- win32 window management and message handler
- Mesh, ObjLoader, Material, GeometryGenerator (basic meshes grid, box etc)
- simple camera implementation
- GUI settings with Dear ImGui.


## 3rd Party libraries:
- Dear ImGui
https://github.com/ocornut/imgui

- tiny obj loader
https://github.com/syoyo/tinyobjloader

- DirectXTex
https://github.com/Microsoft/DirectXTex
  - DirectXTex ScreenCrab
  - DirectXTex WICTextureLoader

- DirectXTK
In some parts using the SimpleMath helpers to wrap DirectXMath SIMD vector/matrix math API
https://github.com/microsoft/DirectXTK
