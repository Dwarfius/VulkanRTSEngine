#version 420

uniform sampler2D tex;

in vec3 normalOut;
in vec2 uvsOut;

layout(location = 0) out vec4 outColor;

void main() 
{
	vec4 sampled = texture(tex, uvsOut);
    //outColor = vec4(sampled.rgb, 1.0);
    outColor = vec4(1., 0., 0., 1.);
}