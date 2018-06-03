#pragma once

#include "MyMathTypes.hpp"


namespace MyUtils
{
	//! @brief  簡易トラックボール処理クラス。<br>
	//! 
	//! http://marina.sys.wakayama-u.ac.jp/~tokoi/?date=20040321 <br>
	class MyTrackball final
	{
		MyMath::Vector2I m_dragStartPos; //!< ドラッグ開始位置。<br>
		MyMath::Vector2F m_scalingVal; //!< マウスの絶対位置→ウィンドウ内での相対位置の換算係数。<br>
		MyMath::QuaternionF m_cq; //!< 回転の初期値（クォータニオン）。<br>
		MyMath::QuaternionF m_tq; //!< ドラッグ中の回転（クォータニオン）。<br>
		MyMath::Matrix4x4F m_rotMatrix; //!< 回転の変換行列。<br>
		bool m_isDragging; //!< ドラッグ中か否か。<br>
	public:
		MyTrackball();
		void OnResize(int w, int h); //!< トラックボール処理の範囲指定。<br>
		void OnMouseDragStart(int x, int y); //!< トラックボール処理の開始。<br>
		void OnMouseDragging(int x, int y); //!< 回転の変換行列の計算。<br>
		void OnMouseDragStop(int x, int y); //!< トラックボール処理の停止。<br>

		//! @brief  回転行列を取得する。<br>
		const MyMath::Matrix4x4F& GetRotationMatrix() const
		{
			return m_rotMatrix;
		}
	};

} // end of namespace
