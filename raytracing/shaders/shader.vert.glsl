#version 130

in vec3 Vertex;
in vec2 TexCoord;

out vec2 fTexCoord;
uniform mat4 Projection;

void main()
{
	fTexCoord = TexCoord;
	gl_Position = Projection * vec4(Vertex, 1.0);
}