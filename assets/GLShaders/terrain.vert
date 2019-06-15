#version 420
#extension GL_ARB_separate_shader_objects : enable

layout (std140, binding = 0) uniform UniformAdapter // TODO: give it a proper name!
{
	mat4 Model;
	mat4 mvp;
};

layout (std140, binding = 1) uniform TerrainAdapter
{
	vec3 GridOrigin;
	int TileSize;
	int GridWidth;
	int GridHeight;
	float YScale; 
};

layout(location = 0) out DataOut 
{
    mediump vec2 TexCoords;
};

void main() 
{
 	// position patch in 2D grid based on instance ID
    int ix = gl_InstanceID % GridWidth;
    int iy = gl_InstanceID / GridHeight;
    vec3 pos = GridOrigin + vec3(float(ix)*TileSize, 0, float(iy)*TileSize);
    TexCoords = vec2(ix / float(GridWidth), iy / float(GridHeight));
    gl_Position = vec4(pos, 1.0);
}
