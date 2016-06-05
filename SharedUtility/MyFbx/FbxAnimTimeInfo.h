#pragma once

#include "MyUtilTypes.hpp"


namespace MyFbx
{
	class MyFbxAnimTimeInfo
	{
	public:
		// コンストラクタ
		MyFbxAnimTimeInfo();

		// デストラクタ
		virtual ~MyFbxAnimTimeInfo();

		// 解析
		void Analyze(const FbxScene* scene);

		static void AnalyzeLegacyTakeInfo(FbxScene* scene);

		FbxTime GetPeriod() const { return m_animPeriod; }

		const TStrArray& GetAnimStackNamesArray() const { return m_animStackNamesArray; }

		int GetFramesPerSec() const { return m_framesPerSec; }

	private:
		TStrArray m_animStackNamesArray; //!< アニメーション スタック名（アニメーション名／テイク名／トラック名）の配列。<br>
		FbxTime m_animPeriod; //!< 単位時間。<br>
		int m_framesPerSec; //!< 1秒当たりフレーム数。<br>
	};
}

