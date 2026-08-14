# Graphics Pipeline

The private OpenGL backend now exercises a real 3D transform chain:

```text
local vec3 position
        ↓ Model (identity)
world position
        ↓ View (bootstrap camera)
camera position
        ↓ Perspective projection
clip space → rasterization → framebuffer → Platform presentation
```

One static VBO contains a unit cube made from local-space triangles. The
Renderer frontend derives bounds and diagnostic colors from visible World
objects; private backend model matrices translate and scale cube instances to
those AABBs. The resulting floor, walls, and boxes provide visible physical
references for FPS navigation without introducing Mesh, Scene, Entity, or
Material APIs. A GLSL 330 Core
vertex shader applies `u_projection * u_view * u_model`; the fragment shader
uses the per-draw diagnostic `u_color` over the dark-purple clear. The frontend
builds HTH matrices, while the backend caches uniform locations and uploads
matrices/colors. Locations are resolved once during initialization and are
required.

Column-major HTH matrices match GLSL storage, so `glUniformMatrix4fv` uses
`GL_FALSE` rather than compensating with a transpose. The projection aspect is
recomputed from framebuffer pixels on resize, while View is refreshed from the
current camera before each render. A zero-sized framebuffer skips
drawing and presentation until dimensions become valid again.

Depth testing is enabled with `GL_DEPTH_TEST` and `GL_LESS` from this milestone
because depth is part of the renderer's 3D semantics.

Shader compilation/link diagnostics and per-context function ownership remain
unchanged. No indices, textures, materials, mesh abstraction, lighting,
animation, filesystem shader loading, or scene API are introduced.
