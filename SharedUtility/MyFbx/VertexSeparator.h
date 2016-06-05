#pragma once

//#include <vector>
//#include <cassert>


namespace MyUtility
{
	//! 新しいインデックスの値を保持するクラス。
	class NewIndex
	{
	public:
		int Index; //!< 新しい頂点のインデックス。
		int ExtraIndex; //!< 追加データのインデックス。

		NewIndex()
			: Index(-1), ExtraIndex(-1)
		{}

		NewIndex(int index, int extraIndex)
			: Index(index), ExtraIndex(extraIndex)
		{}
	};

	//! 異なる追加要素を格納するコンテナ。
	typedef std::vector<NewIndex> TNewIndices;

	//! 頂点の数だけ要素を追加するコンテナ。
	typedef std::vector<TNewIndices> TIndexLookup;


	//! @brief  追加要素に応じて頂点バッファを分離するクラス。
	//! 
	//! @tparam  TV  元の頂点（頂点位置座標）の型。
	//! @tparam  TI  インデックスの型。
	//! @tparam  TE  追加要素（法線、UV 座標など）の型。<br>比較演算子のうち、少なくとも等価演算子 == がオーバーロードされている必要がある。
	template<typename TV, typename TI, typename TE> class VertexSeparator
	{
	private:

		const size_t m_origVertexCount;
		const size_t m_origIndexCount;

		std::vector<TI> m_indexBuffer; //!< 新しいインデックス バッファ。

		TIndexLookup m_indexLookup;
		size_t m_newVertexCount;

		VertexSeparator();
		// コピー禁止
		VertexSeparator(const VertexSeparator&);
		VertexSeparator& operator = (const VertexSeparator&);

	public:
		//! @brief  インデックス バッファの更新に必要なデータの初期化を伴うコンストラクタ。
		//! 
		//! @param[in]  vertexCount  頂点数。
		//! @param[in]  indexCount  インデックス配列の要素数。
		//! @param[in]  pIndexBuffer  インデックス配列。
		VertexSeparator(
			size_t vertexCount,
			size_t indexCount,
			const TI* pIndexBuffer
			)
			: m_origVertexCount(vertexCount)
			, m_origIndexCount(indexCount)
			, m_indexBuffer(pIndexBuffer, pIndexBuffer + indexCount)
			, m_indexLookup(vertexCount)
			, m_newVertexCount(vertexCount)
		{}

		~VertexSeparator()
		{}

		//! @brief  インデックス バッファを更新する。
		//! 
		//! @param[in]  pExtraDataArray  頂点ごとに異なる追加データの配列。<br>データの個数はコンストラクタで渡した indexCount と等しい。
		void UpdateIndex(const TE* pExtraDataArray);

		//! @brief  データを分離して、新たな頂点バッファを作成する。
		//! 
		//! @param[out]  pOutArray  分離した出力データ配列。<br>データの個数は UpdateIndex() 実行後、 GetNewVertexCount() を呼び出すことで得られる。
		//! @param[in]  pInArray  分離する入力データ配列。<br>データの個数はコンストラクタで渡した vertexCount と等しい。
		void SeparateData(TV* pOutArray, const TV* pInArray) const;

		//! @brief  不要なデータを除いた、新しい頂点バッファ用の追加データを取得する。
		//! 
		//! @param[out]  pOutArray  出力先の追加データ配列。<br>データの個数は UpdateIndex() 実行後、 GetNewVertexCount() を呼び出すことで得られる。
		//! @param[in]  pInArray  入力元の追加データ配列。<br>データの個数はコンストラクタで渡した indexCount と等しい。
		void GetNewExtraDataArray(TE* pOutArray, const TE* pInArray) const;

		//! @brief  新しい頂点インデックスから古い頂点インデックスを検索するテーブルを取得する。
		//! 
		//! @param[out]  pOutArray  出力先のインデックス配列。<br>データの個数は UpdateIndex() 実行後、 GetNewVertexCount() を呼び出すことで得られる。
		void GetVertexIndexMap(TI* pOutArray) const;

		size_t GetNewVertexCount() const { return m_newVertexCount; }

		const TI* GetNewIndexBuffer() const { return &m_indexBuffer[0]; }

	}; // VertexSeparator


#pragma region // 実装部 //

	template<typename TE> inline int FindIndexWithSameExtraData(const TE* pExtraDataArray, size_t extraCount, const TNewIndices& newIndices, size_t extraIndex)
	{
		for (size_t i = 0; i < newIndices.size(); ++i)
		{
			// NOTE: 比較演算子のうち、少なくとも等価演算子 == がオーバーロードされている必要がある。
			if (pExtraDataArray[newIndices[i].ExtraIndex] == pExtraDataArray[extraIndex])
			{
				// 見つかった
				return newIndices[i].Index;
			}
		}

		// 見つからなかった
		return -1;
	}


	template<typename TV, typename TI, typename TE> inline void VertexSeparator<TV, TI, TE>::UpdateIndex(const TE* pExtraDataArray)
	{
		m_indexLookup.resize(m_origVertexCount);
		m_newVertexCount = m_origVertexCount;

		// 頂点座標が同じ（インデックスが同じ）で追加のデータが異なる頂点を分離する
		for (size_t i = 0; i < m_origIndexCount; ++i)
		{
			TNewIndices& newIndices = m_indexLookup[m_indexBuffer[i]];
			const int ret = FindIndexWithSameExtraData(pExtraDataArray, m_origIndexCount, newIndices, i);
			if (-1 == ret)
			{
				if (!newIndices.empty())
				{
					// 頂点位置が同じで追加データが異なる
					m_indexBuffer[i] = static_cast<TI>(m_newVertexCount);
					m_newVertexCount++;
				}
				newIndices.push_back(NewIndex(m_indexBuffer[i], int(i)));
			}
			else
			{
				// 同じものが見つかったので新規追加の必要はない
				m_indexBuffer[i] = static_cast<TI>(ret);
			}
		}
	}


	template<typename TV, typename TI, typename TE> inline void VertexSeparator<TV, TI, TE>::SeparateData(TV* pOutArray, const TV* pInArray) const
	{
		_ASSERTE(pOutArray);
		_ASSERTE(pInArray);

		for (size_t i = 0; i < m_indexLookup.size(); ++i)
		{
			const TNewIndices& newIndices = m_indexLookup[i];

			for (size_t j = 0; j < newIndices.size(); ++j)
			{
				pOutArray[newIndices[j].Index] = pInArray[i];
			}
		}
	}


	template<typename TV, typename TI, typename TE> inline void VertexSeparator<TV, TI, TE>::GetNewExtraDataArray(TE* pOutArray, const TE* pInArray) const
	{
		_ASSERTE(pOutArray);
		_ASSERTE(pInArray);

		for (size_t i = 0; i < m_indexLookup.size(); ++i)
		{
			const TNewIndices& newIndices = m_indexLookup[i];

			for (size_t j = 0; j < newIndices.size(); ++j)
			{
				pOutArray[newIndices[j].Index] = pInArray[newIndices[j].ExtraIndex];
			}
		}
	}


	template<typename TV, typename TI, typename TE> inline void VertexSeparator<TV, TI, TE>::GetVertexIndexMap(TI* pOutArray) const
	{
		_ASSERTE(pOutArray);

		for (size_t i = 0; i < m_indexLookup.size(); ++i)
		{
			const TNewIndices& newIndices = m_indexLookup[i];

			for (size_t j = 0; j < newIndices.size(); ++j)
			{
				pOutArray[newIndices[j].Index] = i;
			}
		}
	}

#pragma endregion

} // MyUtility
