# Graphics Pipeline

The private OpenGL backend now exercises a real 3D transform chain:

```text
local vec3 position + vec2 UV
        ↓ Model (identity)
world position
        ↓ View (bootstrap camera)
camera position
        ↓ Perspective projection
clip space → rasterization → framebuffer → Platform presentation
```

One static VBO contains a unit cube made from local-space triangles with a
full `[0,1]` UV square on each face. Before Renderer initialization, World
visual classes pass through the temporary bootstrap material binding and
external `hthmaterial`/PPM resources. Renderer receives only resolved bounds,
base color, and optional RGB8 pixels; private backend model matrices translate
and scale cube instances to those AABBs.

A GLSL 330 Core vertex shader applies
`u_projection * u_view * u_model` and forwards UV. The fragment shader emits
base color or `texture(u_base_texture, uv) * base_color` according to a
per-draw uniform. The backend caches all uniform locations, uploads each
texture once during initialization, and binds one texture unit while drawing.
The externally stored palette and diagnostic textures provide visible physical
references without introducing public Mesh, Scene, Entity, or Material APIs.

Column-major HTH matrices match GLSL storage, so `glUniformMatrix4fv` uses
`GL_FALSE` rather than compensating with a transpose. The projection aspect is
recomputed from framebuffer pixels on resize, while View is refreshed from the
current camera before each render. A zero-sized framebuffer skips
drawing and presentation until dimensions become valid again.

Depth testing is enabled with `GL_DEPTH_TEST` and `GL_LESS` from this milestone
because depth is part of the renderer's 3D semantics.

Shader compilation/link diagnostics and per-context function ownership remain
unchanged. No indices, mesh abstraction, lighting, PBR, mipmaps, material
cache, filesystem access in Renderer, animation, or scene API are introduced.
