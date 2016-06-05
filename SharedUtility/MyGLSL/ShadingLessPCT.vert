//#version 440
//#version 330
//#version 140
//#version 120

#if 0
uniform mat4 UniWorldViewProj;
#endif

layout(location = 0) in vec3 VsInPosition;
layout(location = 1) in vec4 VsInColor;
layout(location = 2) in vec2 VsInTexCoord;

out vec4 mygl_Color;
out vec2 mygl_TexCoord0;

void main(void)
{
	vec4 vPosition4 = vec4(VsInPosition, 1.0);
	gl_Position = UniWorldViewProj * vPosition4;
#if 0
	gl_FrontColor = VsInColor;
	gl_TexCoord[0] = vec4(VsInTexCoord.x, VsInTexCoord.y, 0.0, 0.0);
#else
	mygl_Color = VsInColor;
	mygl_TexCoord0 = VsInTexCoord;
#endif
}
