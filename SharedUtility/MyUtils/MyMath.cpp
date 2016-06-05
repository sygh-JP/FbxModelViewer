#include "stdafx.h"
#include "DebugNew.h"
#include "MyMath.hpp"


namespace MyMath
{

	//! @brief  ガウス分布の重み配列（ただしカーネル中心値と右半分）を作成する。<br>
	void CreateGaussianWeightsArray(float outArray[], size_t arraySize, size_t kernelHalf, float dispersion)
	{
		// http://www.t-pot.com/program/79_Gauss/index.html
		// この計算は正しいのか？　√2π で割ってないけどいいの？

		_ASSERTE(arraySize > kernelHalf);
		outArray[0] = 1;
		float total = outArray[0];
		for (size_t i = 1; i <= kernelHalf; ++i)
		{
			outArray[i] = expf(-0.5f * static_cast<float>(i * i) / dispersion);
			// 中心以外は、対称的に2回同じ係数を使うので2倍する。
			total += 2.0f * outArray[i];
		}
		// 正規化。
		for (size_t i = 0; i <= kernelHalf; ++i)
		{
			outArray[i] /= total;
		}
	}


	template<typename T> bool CompareArrays(const std::vector<T>& a, const std::vector<T>& b)
	{
		if (a.size() != b.size())
		{
			return false;
		}
		// memcmp は使わない。
		for (size_t i = 0; i < a.size(); ++i)
		{
			if (a[i] != b[i])
			{
				return false;
			}
		}
		return true;
	}

	bool CompareGradientColorStopArrays(const TMyGradientColorStopArray& a, const TMyGradientColorStopArray& b)
	{
		return CompareArrays(a, b);
	}

	bool CompareGradientOpacityStopArrays(const TMyGradientOpacityStopArray& a, const TMyGradientOpacityStopArray& b)
	{
		return CompareArrays(a, b);
	}

} // end of namespace
