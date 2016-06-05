#include "stdafx.h"
#include "MyAppSettings.hpp"


namespace MyApp
{
	void MyCameraSettings::Initialize()
	{
		this->m_vTranslation = MyMath::ZERO_VECTOR3F;
		this->m_vRotation = MyMath::ZERO_VECTOR3F;

		//this->m_vCameraEye = MyMath::Vector3F(0, 0, +1500);

		//this->FovAngleDegree = 45.0f;
		this->FovAngleDegree = 20.0f;
		this->ClipPlaneNearZ = 0.1f;
		this->ClipPlaneFarZ = 2500.0f;
	}
}
