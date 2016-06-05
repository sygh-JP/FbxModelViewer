#pragma once

#include "MyFbx.h"


namespace MyFbx
{
	typedef std::vector<MyMath::Vector2F> TUVArray;

	class MyFbxUVAnalyzer
	{
	public:
		typedef std::shared_ptr<MyFbxUVAnalyzer> TSharedPtr;
		typedef std::shared_ptr<const MyFbxUVAnalyzer> TConstSharedPtr;
	public:
		MyFbxUVAnalyzer() {}

		virtual ~MyFbxUVAnalyzer() {}

		// UV解析
		bool Analyze(const FbxLayer* layer);

		const TUVArray& GetDiffuseUVCoordArray() const { return m_diffuseUVArray; }

		void SetDiffuseUVCoordArray(const TUVArray& newUVArray) { m_diffuseUVArray = newUVArray; }

		LayerModeData GetDiffuseUVLayerModeData() const { return m_diffuseLayerModeData; }

	private:
		TUVArray m_diffuseUVArray; //!< ディフューズ UV。<br>
		TUVArray m_specularUVArray; //!< スペキュラー UV。<br>
		TUVArray m_ambientUVArray; //!< アンビエント UV。<br>
		TUVArray m_emissiveUVArray; //!< エミッシブ UV。<br>
		LayerModeData m_diffuseLayerModeData;
		LayerModeData m_specularLayerModeData;
		LayerModeData m_ambientLayerModeData;
		LayerModeData m_emissiveLayerModeData;
	};
} // end of namespace

