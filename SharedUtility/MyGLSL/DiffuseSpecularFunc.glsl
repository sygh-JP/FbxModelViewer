//#version 440
//#version 330

float CalcLambertDiffuse(vec3 wnormal, vec3 light)
{
	return max(dot(wnormal, light), 0.0);
}

float CalcHalfLambertDiffuse(vec3 wnormal, vec3 light)
{
	float lambert = CalcLambertDiffuse(wnormal, light);
	lambert = lambert * 0.5 + 0.5;
	return lambert * lambert;
}

float CalcBlinnPhongSpecular(vec3 wnormal, vec3 halfway, float specularPower)
{
	return pow(max(dot(wnormal, halfway), 0.0), specularPower);
}

float CalcSchlickFresnelTerm(float fresnel0, float dotLH)
{
	return fresnel0 + (1.0 - fresnel0) * pow(1.0 - dotLH, 5.0);
}

float CalcSchlickFresnelTerm(float fresnel0, vec3 light, vec3 halfway)
{
	return CalcSchlickFresnelTerm(fresnel0, dot(light, halfway));
}

vec3 CalcSchlickFresnelTerm(vec3 fresnel0, float dotLH)
{
	return fresnel0 + (1.0 - fresnel0) * pow(1.0 - dotLH, 5.0);
}

vec3 CalcSchlickFresnelTerm(vec3 fresnel0, vec3 light, vec3 halfway)
{
	return CalcSchlickFresnelTerm(fresnel0, dot(light, halfway));
}
