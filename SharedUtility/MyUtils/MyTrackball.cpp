#include "stdafx.h"
#include "MyMath.hpp"
#include "MyTrackball.h"


namespace MyUtil
{
	MyTrackball::MyTrackball()
		: m_isDragging()
		, m_cq(1, 0, 0, 0)
	{
	}

	void MyTrackball::OnResize(int w, int h)
	{
		m_scalingVal.x = 1.0f / w;
		m_scalingVal.y = 1.0f / h;
	}

	void MyTrackball::OnMouseDragStart(int x, int y)
	{
		m_isDragging = true;

		m_dragStartPos.x = x;
		m_dragStartPos.y = y;
	}

	void MyTrackball::OnMouseDragging(int x, int y)
	{
		if (m_isDragging)
		{
			const MyMath::Vector2F diff(
				(x - m_dragStartPos.x) * m_scalingVal.x,
				(y - m_dragStartPos.y) * m_scalingVal.y);

			const float a = MyMath::GetVector2Length(diff);

			if (FLT_EPSILON < a)
			{
				const float ar = a * MyMath::F_PI;
				const float as = sin(ar) / a;
				const MyMath::QuaternionF dq(cos(ar), diff.y * as, diff.x * as, 0);

				// クォータニオンを掛けて回転を合成。
				//MyMath::JointQuaternion(&m_tq, &dq, &m_cq); // NG。
				MyMath::JointQuaternion(&m_tq, &m_cq, &dq);

				// クォータニオンから回転の変換行列を求める。
				MyMath::CreateMatrixFromRotationQuaternion(&m_rotMatrix, &m_tq);
			}
		}
	}

	void MyTrackball::OnMouseDragStop(int x, int y)
	{
		this->OnMouseDragging(x, y);

		m_cq = m_tq;

		m_isDragging = false;
	}

} // end of namespace
