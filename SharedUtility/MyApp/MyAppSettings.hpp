#pragma once

#include "MyMath.hpp"


// アプリケーション固有の設定情報。
namespace MyApp
{
	//! @brief  本来はそれぞれ分けてクラス化すべき情報だが、<br>
	//! とりあえず変換行列等作成用の Direct3D/OpenGL 共通カメラ系パラメータとしてまとめる。<br>
	//! 3DCG ソフトではパースペクティブ カメラなどと称して一括管理されていることが多い。<br>
	class MyCameraSettings final
	{
	public:
		MyMath::Vector3F m_vTranslation; //!< ワールドの平行移動量。<br>
		MyMath::Vector3F m_vRotation; //!< ワールドの回転量（Pitch, Yaw, Roll）[rad]。<br>

#pragma region // カメラ パラメータ。//
		MyMath::Vector3F m_vCameraEye; //!< カメラ視点位置。<br>
		MyMath::Vector3F m_vCameraAt; //!< カメラ注視点位置。<br>
		MyMath::Vector3F m_vCameraUp; //!< カメラ上向きベクトル。<br>
#pragma endregion

#pragma region // プロジェクション パラメータ。//
	private:
		bool m_isPersProj;

	public:
		float FovAngleDegree;

		uint32_t m_screenWidth;
		uint32_t m_screenHeight;

		float ClipPlaneNearZ;
		float ClipPlaneFarZ;
#pragma endregion

	public:
		MyCameraSettings()
			: m_vTranslation(0, 0, 0)
			, m_vRotation(0, 0, 0)
#ifdef USE_LEFT_HAND_COORD_SYS
			, m_vCameraEye(0, 0, -1000)
#else
			, m_vCameraEye(0, 0, +1000)
#endif
			, m_vCameraAt(0, 0, 0)
			, m_vCameraUp(0, 1, 0)
			, m_isPersProj(true)
			, FovAngleDegree()
			, m_screenWidth()
			, m_screenHeight()
			, ClipPlaneNearZ()
			, ClipPlaneFarZ()
		{
		}

	public:
		float GetScreenWidthF() const { return static_cast<float>(m_screenWidth); }
		float GetScreenHeightF() const { return static_cast<float>(m_screenHeight); }

		float GetAspectRatio() const
		{ return this->GetScreenWidthF() / this->GetScreenHeightF(); }

		bool GetIsPerspetiveProjMode() const { return m_isPersProj; }
		bool GetIsOrthographicProjMode() const { return !m_isPersProj; }

		void Initialize();

	public:
		void GetMatrixWorld(MyMath::MatrixF* pWorld) const
		{
			// スケーリングは考慮しない。
#if 0
			D3DXMatrixRotationYawPitchRoll(pWorld,
				m_vRotation.y,
				m_vRotation.x,
				m_vRotation.z);
#else
			MyMath::CreateMatrixRotationZXY(pWorld, &m_vRotation);
#endif
			pWorld->_41 = m_vTranslation.x;
			pWorld->_42 = m_vTranslation.y;
			pWorld->_43 = m_vTranslation.z;
		}

		//! @brief  転置行列を求めるバージョン。<br>
		void GetTrMatrixWorld(MyMath::MatrixF* pWorld) const
		{
			// スケーリングは考慮しない。
#if 0
			D3DXMatrixRotationYawPitchRoll(pWorld,
				m_vRotation.y,
				m_vRotation.x,
				m_vRotation.z);
#else
			MyMath::CreateTrMatrixRotationPitchYawRoll(pWorld, &m_vRotation);
#endif
			pWorld->_14 = m_vTranslation.x;
			pWorld->_24 = m_vTranslation.y;
			pWorld->_34 = m_vTranslation.z;
		}

		void GetMatrixView(MyMath::MatrixF* pView) const
		{
#ifdef USE_LEFT_HAND_COORD_SYS
			MyMath::CreateMatrixLookAtLH(pView, &m_vCameraEye, &m_vCameraAt, &m_vCameraUp);
#else
			MyMath::CreateMatrixLookAtRH(pView, &m_vCameraEye, &m_vCameraAt, &m_vCameraUp);
#endif
		}

		//! @brief  転置行列を求めるバージョン。<br>
		void GetTrMatrixView(MyMath::MatrixF* pView) const
		{
#ifdef USE_LEFT_HAND_COORD_SYS
			MyMath::CreateTrMatrixLookAtLH(pView, &m_vCameraEye, &m_vCameraAt, &m_vCameraUp);
#else
			MyMath::CreateTrMatrixLookAtRH(pView, &m_vCameraEye, &m_vCameraAt, &m_vCameraUp);
#endif
		}

		void GetMatrixProjection(MyMath::MatrixF* pProj) const
		{
#ifdef USE_LEFT_HAND_COORD_SYS
			if (m_isPersProj)
			{
				MyMath::CreateMatrixPerspectiveFovLH(pProj,
					MyMath::ToRadian(FovAngleDegree), this->GetAspectRatio(), ClipPlaneNearZ, ClipPlaneFarZ);
			}
			else
			{
				MyMath::CreateMatrixOrthoLH(pProj,
					this->GetScreenWidthF(), this->GetScreenHeightF(), ClipPlaneNearZ, ClipPlaneFarZ);
			}
#else
			if (m_isPersProj)
			{
				MyMath::CreateMatrixPerspectiveFovRH(pProj,
					MyMath::ToRadian(FovAngleDegree), this->GetAspectRatio(), this->ClipPlaneNearZ, this->ClipPlaneFarZ);
			}
			else
			{
				MyMath::CreateMatrixOrthoRH(pProj,
					this->GetScreenWidthF(), this->GetScreenHeightF(), this->ClipPlaneNearZ, this->ClipPlaneFarZ);
			}
#endif
		}

		//! @brief  転置行列を求めるバージョン。<br>
		void GetTrMatrixProjection(MyMath::MatrixF* pProj) const
		{
#ifdef USE_LEFT_HAND_COORD_SYS
			if (m_isPersProj)
			{
				MyMath::CreateTrMatrixPerspectiveFovLH(pProj,
					MyMath::ToRadian(FovAngleDegree), this->GetAspectRatio(), ClipPlaneNearZ, ClipPlaneFarZ);
			}
			else
			{
				MyMath::CreateTrMatrixOrthoLH(pProj,
					this->GetScreenWidthF(), this->GetScreenHeightF(), ClipPlaneNearZ, ClipPlaneFarZ);
			}
#else
			if (m_isPersProj)
			{
				MyMath::CreateTrMatrixPerspectiveFovRH(pProj,
					MyMath::ToRadian(FovAngleDegree), this->GetAspectRatio(), this->ClipPlaneNearZ, this->ClipPlaneFarZ);
			}
			else
			{
				MyMath::CreateTrMatrixOrthoRH(pProj,
					this->GetScreenWidthF(), this->GetScreenHeightF(), this->ClipPlaneNearZ, this->ClipPlaneFarZ);
			}
#endif
		}

	public:
		void GetMatrixWorldViewProj(_Out_ MyMath::MatrixF* pMatWorld, _Out_ MyMath::MatrixF* pMatView, _Out_ MyMath::MatrixF* pMatProj,
			_Out_ MyMath::MatrixF* pMatWdVw, _Out_ MyMath::MatrixF* pMatVwPj, _Out_ MyMath::MatrixF* pMatWdVwPj) const
		{
			this->GetMatrixWorld(pMatWorld);
			this->GetMatrixView(pMatView);
			this->GetMatrixProjection(pMatProj);
			// World * View
			MyMath::MultiplyMatrix(pMatWdVw, pMatWorld, pMatView);
			// View * Proj
			MyMath::MultiplyMatrix(pMatVwPj, pMatView, pMatProj);
			// World * View * Proj
			MyMath::MultiplyMatrix(pMatWdVwPj, pMatWdVw, pMatProj);
		}

		//! @brief  転置行列を求めるバージョン。<br>
		void GetTrMatrixWorldViewProj(_Out_ MyMath::MatrixF* pMatWorld, _Out_ MyMath::MatrixF* pMatView, _Out_ MyMath::MatrixF* pMatProj,
			_Out_ MyMath::MatrixF* pMatWdVw, _Out_ MyMath::MatrixF* pMatVwPj, _Out_ MyMath::MatrixF* pMatWdVwPj) const
		{
			this->GetTrMatrixWorld(pMatWorld);
			this->GetTrMatrixView(pMatView);
			this->GetTrMatrixProjection(pMatProj);
			// (World * View)^T
			MyMath::MultiplyMatrix(pMatWdVw, pMatView, pMatWorld);
			// (View * Proj)^T
			MyMath::MultiplyMatrix(pMatVwPj, pMatProj, pMatView);
			// (World * View * Proj)^T
			MyMath::MultiplyMatrix(pMatWdVwPj, pMatProj, pMatWdVw);
		}

		void GetMatrixViewport(_Out_ MyMath::MatrixF* pMatViewport) const
		{
			MyMath::CreateMatrixViewport(pMatViewport, 0, 0, this->GetScreenWidthF(), this->GetScreenHeightF(), 0, 1); 
		}

		void GetMatrixTransformWorldCoordToScreenCoord(_Out_ MyMath::MatrixF* pMatTransform) const
		{
			MyMath::MatrixF mView, mProj, mVP;
			this->GetMatrixView(&mView);
			this->GetMatrixProjection(&mProj);
			this->GetMatrixViewport(&mVP);
			MyMath::MultiplyMatrix(pMatTransform, &mView, &mProj, &mVP);
		}

		void GetMatrixTransformScreenCoordToWorldCoord(_Out_ MyMath::MatrixF* pMatTransform) const
		{
			this->GetMatrixTransformWorldCoordToScreenCoord(pMatTransform);
			MyMath::InverseMatrix(pMatTransform, pMatTransform);
		}

		void SetCameraEye(const MyMath::Vector3F& eyePos)
		{
			// 視点位置 Eye と注視点 At が接近しすぎると、XNA Math や DirectXMath の XMMatrixLookAtLH() / XMMatrixLookAtRH() が失敗する。
			const float limitLenSq = 0.01f;
			if (MyMath::GetVector3DistanceSq(m_vCameraAt, eyePos) > limitLenSq)
			{
				m_vCameraEye = eyePos;
			}
		}

		void PanCamera(MyMath::Vector2F shiftInPix)
		{
#if 0
			MyMath::MatrixF matScreenToWorld;
			this->GetMatrixTransformScreenCoordToWorldCoord(&matScreenToWorld);
			MyMath::Vector3F p0(0, 0, m_vCameraEye.z);
			MyMath::Vector3F p1(shiftInPix.x, shiftInPix.y, m_vCameraEye.z);
			MyMath::TransformVectorByMatrix(&p0, &p0, &matScreenToWorld);
			MyMath::TransformVectorByMatrix(&p1, &p1, &matScreenToWorld);
			m_vCameraAt.x += (p1.x - p0.x);
			m_vCameraAt.y += (p1.y - p0.y);
#else
			m_vCameraAt.x += shiftInPix.x;
			m_vCameraAt.y += shiftInPix.y;
#endif
		}
	};


	class FpsCounter
	{
		UINT m_frameCounter;
		double m_elapsedTimeSec;
		double m_fpsValue;

	public:
		FpsCounter()
			: m_frameCounter()
			, m_elapsedTimeSec()
			, m_fpsValue()
		{
		}

		void AdvanceFrameCounter() { ++m_frameCounter; }
		UINT GetFrameCounter() const { return m_frameCounter; }
		// 1フレームで経過した実時間を加えていく。積算値が FPS の計算に使われる。
		void AddElapsedTimeSec(double timeSec) { m_elapsedTimeSec += timeSec; }
		double GetElapsedTimeSec() const { return m_elapsedTimeSec; }
		void Reset() { m_frameCounter = 0; m_elapsedTimeSec = 0; }

		void UpdateFpsValue() { m_fpsValue = m_frameCounter / m_elapsedTimeSec; }
		double GetFpsValue() const { return m_fpsValue; }
	};


	//! @brief  Direct3D/OpenGL 共通の画面表示設定などを管理する。<br>
	class MyCommonSettings
	{
	public:
		bool DisplaysCoordAxes;
		MyMath::Vector4F BackColor;
		const MyMath::Vector3F MainLightDir;

		MyMath::Vector3F MainLightRotation;

	public:
		MyCommonSettings()
			: DisplaysCoordAxes()
			, BackColor(0,0,0,0)
#ifdef USE_LEFT_HAND_COORD_SYS
			, MainLightDir(0, 0, +1)
#else
			, MainLightDir(0, 0, -1)
#endif
			, MainLightRotation(0, 0, 0)
		{
		}

	private:
		void GetD3DMatrixMainLightRotation(MyMath::MatrixF* pRotation) const
		{
#if 0
			D3DXMatrixRotationYawPitchRoll(pRotation,
				MainLightRotation.y,
				MainLightRotation.x,
				MainLightRotation.z);
#else
			MyMath::CreateMatrixRotationZXY(pRotation, &this->MainLightRotation);
#endif
		}

	private:
		void RotateLightDir(MyMath::Vector4F* pOut, const MyMath::Vector3F* pIn) const
		{
			MyMath::MatrixF mLightRotation;
			this->GetD3DMatrixMainLightRotation(&mLightRotation);
			//D3DXVec3Transform(pOut, pIn, &mLightRotation);
			MyMath::TransformVectorByMatrix(pOut, pIn, &mLightRotation);
		}

		void RotateLightDir(MyMath::Vector3F* pOut, const MyMath::Vector3F* pIn) const
		{
			MyMath::MatrixF mLightRotation;
			this->GetD3DMatrixMainLightRotation(&mLightRotation);
			//D3DXVec3Transform(pOut, pIn, &mLightRotation);
			MyMath::TransformVectorByMatrix(pOut, pIn, &mLightRotation);
		}

	public:
		void GetRotatedLightDir(MyMath::Vector4F* pOut) const
		{
			this->RotateLightDir(pOut, &this->MainLightDir);
		}

		void GetRotatedLightDir(MyMath::Vector3F* pOut) const
		{
			this->RotateLightDir(pOut, &this->MainLightDir);
		}
	};


	//! @brief  主にエフェクト設定などを管理する。<br>
	class MyEffectSettings
	{
	public:
		bool DisplaysWaveFront;
		bool EnablesToonShading;
		bool EnablesToonInk;
		bool EnablesImageBasedFur;
		bool EnablesBloomEffect;

	public:
		MyEffectSettings()
			: DisplaysWaveFront()
			, EnablesToonShading()
			, EnablesToonInk()
			, EnablesImageBasedFur()
			, EnablesBloomEffect()
		{
		}
	};
} // end of namespace
