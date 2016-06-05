#pragma once

// アプリケーション固有の頂点型を定義する。

#include "MyMath.hpp"

namespace MyVertexTypes
{
	class MyFlakeVertex final
	{
	public:
		MyMath::Vector3F Position;
		MyMath::QuaternionF Attitude;
	};
}
