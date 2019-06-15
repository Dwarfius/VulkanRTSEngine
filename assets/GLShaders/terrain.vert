#version 420

layout(location = 0) in vec3 position;

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
};

void main() 
{
 	// position patch in 2D grid based on instance ID
    int ix = gl_InstanceID % GridWidth;
    int iy = gl_InstanceID / GridHeight;
    vec3 pos = GridOrigin + vec3(float(ix)*TileSize, 0, float(iy)*TileSize);
    gl_Position = vec4(pos, 1.0);
}
