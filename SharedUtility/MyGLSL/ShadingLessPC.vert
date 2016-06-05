//#version 440
//#version 330
//#version 120

#if 0
//uniform mat4 UniWorld;
//uniform mat4 UniView;
//uniform mat4 UniProjection;
uniform mat4 UniWorldViewProj;
#endif

layout(location = 0) in vec3 VsInPosition;
layout(location = 1) in vec4 VsInColor;

void main(void)
{
	vec4 vPosition4 = vec4(VsInPosition, 1.0);
	gl_Position = UniWorldViewProj * vPosition4;
	// gl_Position に関しては、あとは w = 1 に射影する必要はない。勝手によろしくやってくれる。HLSL の SV_Position と同じ。
	//gl_Position = UniProjection * UniView * UniWorld * vPosition4;
	//gl_Position = gl_ModelViewProjectionMatrix * vPosition4;
	gl_FrontColor = VsInColor;
}
