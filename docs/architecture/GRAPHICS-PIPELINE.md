# Graphics Pipeline Foundation v0.1.5

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

One static VBO contains three-component local-space positions. A GLSL 330 Core
vertex shader applies `u_projection * u_view * u_model`; the fragment shader
retains the bootstrap pink-red color over the dark-purple clear. The frontend
builds HTH matrices, while the backend only caches uniform locations and uploads
them. Locations are resolved once during initialization and are required.

Column-major HTH matrices match GLSL storage, so `glUniformMatrix4fv` uses
`GL_FALSE` rather than compensating with a transpose. The projection aspect is
recomputed from framebuffer pixels on resize. A zero-sized framebuffer skips
drawing and presentation until dimensions become valid again.

Depth testing is enabled with `GL_DEPTH_TEST` and `GL_LESS` from this milestone
because depth is part of the renderer's 3D semantics, even though the bootstrap
contains only one triangle.

Shader compilation/link diagnostics and per-context function ownership remain
unchanged. No indices, textures, materials, mesh abstraction, lighting,
animation, filesystem shader loading, or scene API are introduced.
