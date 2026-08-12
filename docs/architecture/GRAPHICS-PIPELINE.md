# Graphics Pipeline Foundation v0.1.4

The first programmable pipeline remains private to the OpenGL backend:

```text
clip-space vertices
        ↓
static vertex buffer
        ↓
vertex array / position attribute 0
        ↓
GLSL 330 vertex shader
        ↓
rasterization
        ↓
GLSL 330 fragment shader
        ↓
framebuffer
        ↓
Platform presentation
```

Three vertices contain only two-component positions expressed directly in
normalized device coordinates. A single static VBO uploads them during
Renderer initialization. A single VAO records the location-zero attribute
layout. Each frame binds one program and VAO and submits one triangle draw.

The fragment shader produces a constant pink-red color over the existing dark
purple diagnostic clear. Both shader sources are embedded static strings for
bootstrap only; they do not establish the future asset/shader architecture.

Shader compilation and program linking validate status and report complete
driver logs when available. Shader objects are deleted after link. The backend
owns the remaining program, VAO, and VBO and deletes them before destroying the
graphics context.

No indices, textures, uniforms, matrices, camera, materials, mesh abstraction,
lighting, animation, or filesystem shader loading are part of v0.1.4.
