#include "stdafx.h"
#include "MyOGLDynamicFontRects.hpp"


namespace MyOGL
{

	bool MyDynamicManyRectsBase::Create(uint32_t rectCount, uint32_t strideInBytes, const void* pInitialData)
	{
		_ASSERTE(m_vertexBuffer.get() == 0);
		//_ASSERTE(pInitialData != nullptr); // OpenGL では D3D と違って頂点バッファ初期化用データに NULL も一応指定可能。ただしおそらく不定データになるはず。
		_ASSERTE(rectCount > 0);
		_ASSERTE(strideInBytes > 0);

		const uint32_t vertexCount = rectCount * 4;

		const uint32_t byteWidth = vertexCount * strideInBytes;
		const void* pSrcData = pInitialData;

		m_vertexBuffer = Factory::CreateOneBufferPtr();
		_ASSERTE(m_vertexBuffer.get() != 0);
		if (m_vertexBuffer.get() == 0)
		{
			return false;
		}

		glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer.get());
		// GL_STREAM_DRAW と GL_DYNAMIC_DRAW の違いが不明。
		// 前者はフレームあたり1回程度の使い捨て用途、後者はフレームあたり2回以上書き換える用途に最適化されている？
		// http://extns.cswiki.jp/index.php?OpenGL%2F%E3%83%90%E3%83%83%E3%83%95%E3%82%A1
		// HACK: 1フレームで頂点バッファを何度も書き換える場合、都度 glFlush() や glFinish() を呼ばないといけない環境（ドライバー）も存在する。
		// ある Android 実機では glFinish() なしで glBufferSubData() を1フレーム内で続けて呼び出すと、
		// 意図した描画結果にならない（頂点バッファの内容が正しく更新されない）現象に遭遇した。
		// なお、iPhone や Android などのモバイル環境では、
		// 比較的余裕のある NVIDIA GeForce や AMD Radeon などの PC 向け GPU アーキテクチャ＆ドライバーとは違って、
		// パフォーマンス最優先のためにタイルベースの遅延レンダラー（Tile-Based Deferred Rendering, TBDR）が採用されているらしく、
		// glTexSubImage2D() を1フレーム内で何度も呼び出すと性能が低下するらしい。
		// http://blog.livedoor.jp/abars/archives/51879199.html
		// フォント描画・スプライト描画に関しては、
		// ID3DXSprite, ID3DX10Sprite や、XNA の SpriteBatch などのように、描画タスクをキューイングするバッチ処理を実装して、
		// 頂点バッファの書き換え回数を減らすようにしたほうがよさげ。
		// ID3DXSprite::Draw() は D3D9 テクスチャを、ID3DX10Sprite::DrawXXX() は D3D10 テクスチャの SRV をパラメータにとる。
		// ID3DXFont::DrawText(), ID3DX10Font::DrawText() はスプライトのインターフェイスを受け取って効率化することも可能になっている。
		// DirectX Tool Kit（DirectXTK, DXTK）には SpriteBatch クラスがオープンソース実装されているので、それを参考にする手もある。
		// ただ、Direct3D 定数バッファ相当の OpenGL Uniform Block は通例頻繁にバッファ内容を書き換える用途で使用されるはずのため、
		// ID3D11DeviceContext::UpdateSubresource() 相当機能が glBufferSubData() だとすれば、
		// ES 3.0 対応モバイルでも glFinish() を呼ぶことなく glBufferSubData() だけできちんと更新してくれないと困るのだが……
		// OpenGL 1.5 では D3D10/11 の Map(), Unmap() に似た glMapBuffer(), glUnmapBuffer() が導入されているが、
		// モバイルでは glBufferSubData() 同様もしくはそれ以上に性能低下すると思われる。
		// ちなみに glInvalidateBufferData(), glInvalidateBufferSubData() は何のために存在する？

		glBufferData(GL_ARRAY_BUFFER, byteWidth, pSrcData, GL_DYNAMIC_DRAW);
		//glBufferData(GL_ARRAY_BUFFER, byteWidth, pSrcData, GL_STREAM_DRAW);
		glBufferStorage(GL_ARRAY_BUFFER, byteWidth, pSrcData, GL_DYNAMIC_STORAGE_BIT);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// NVIDIA ドライバーを更新すると、
		// glBufferData(), glBufferStorage() 呼び出し箇所などで、
		// Source="OpenGL API", Type="Other", ID=131185, Severity=Low, Message="Buffer detailed info: Buffer object 1 (bound to GL_ARRAY_BUFFER_ARB, usage hint is GL_STATIC_DRAW) will use VIDEO memory as the source for buffer object operations."
		// などの診断メッセージが出力されるようになる。


		std::vector<TIndex> indexArray(rectCount * 6);
		for (size_t i = 0; i < rectCount; ++i)
		{
			// CCW
			indexArray[i * 6 + 0] = TIndex(0 + (i * 4));
			indexArray[i * 6 + 1] = TIndex(2 + (i * 4));
			indexArray[i * 6 + 2] = TIndex(1 + (i * 4));
			indexArray[i * 6 + 3] = TIndex(1 + (i * 4));
			indexArray[i * 6 + 4] = TIndex(2 + (i * 4));
			indexArray[i * 6 + 5] = TIndex(3 + (i * 4));
		}
		m_indexArray.swap(indexArray);

		m_rectCount = rectCount;
		m_vertexBufferSizeInBytes = byteWidth;

		return true;
	}

	bool MyDynamicManyRectsBase::ReplaceVertexData(const void* pSrcData)
	{
		_ASSERTE(m_vertexBuffer.get() != 0);
		_ASSERTE(pSrcData != nullptr);

		const uint32_t byteWidth = m_vertexBufferSizeInBytes;

		glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer.get());
		// 確保済みのバッファの内容を書き換える場合、glBufferData() ではなく glBufferSubData() を使う。
		glBufferSubData(GL_ARRAY_BUFFER, 0, byteWidth, pSrcData);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		return true;
	}


	bool MyDynamicFontRects::UpdateVertexBufferByString(LPCWSTR pString,
		MyMath::Vector2F posInPixels,
		const MyMath::Vector4F& upperColor, const MyMath::Vector4F& lowerColor,
		long fontHeight, bool usesFixedFeed,
		uint32_t fontTexWidth, uint32_t fontTexHeight,
		const MyMath::TCharCodeUVMap& codeUVMap)
	{
		_ASSERTE(pString != nullptr);
		m_stringLength = 0;
		float offsetX = 0;
		TVertex* pVertexArray = &m_vertexArray[0]; // ローカル変数にキャッシュして高速化。
		for (int i = 0; pString[i] != 0 && i < MaxCharacterCount; ++i, ++m_stringLength)
		{
			// HACK: このアルゴリズム（というか単一の wchar_t キー）だと、サロゲート ペアにはどうしても対応不可能。
			const size_t charCode = static_cast<size_t>(pString[i]);
			if (charCode < codeUVMap.size())
			{
				const auto codeUV = codeUVMap[charCode];
				const float uvLeft   = static_cast<float>(codeUV.X) / fontTexWidth;
				const float uvTop    = static_cast<float>(codeUV.Y) / fontTexHeight;
				const float uvRight  = static_cast<float>(codeUV.GetRight()) / fontTexWidth;
				const float uvBottom = static_cast<float>(codeUV.GetBottom()) / fontTexHeight;
				const float uvWidth   = codeUV.Width;
				const float uvHeight  = codeUV.Height;
				// LT, RT, LB, RB.（0, 1, 2 は左手系の定義順）
				const size_t index0 = i * 4 + 0;
				// 文字の水平方向送り（カーニングは考慮しないが、プロポーショナルの場合は文字幅）。
				// スペースの文字送りも考慮する。
				// HUD 系は常にモノスペースのほうがいいこともある。
				// 非 ASCII はテクスチャ作成時にフォント メトリックから取得した文字幅にする。
				// ヨーロッパ言語など、非 ASCII でもモノスペース時は半角幅のほうがいい文字もあるが、それは考慮しない。
				// したがって、メソッドのフラグで等幅指定されたら、ASCII のみ半角幅とし、
				// 非 ASCII はフォント メトリックから取得した文字幅を使うようにする。
				const float feed = (usesFixedFeed && iswascii(pString[i])) ? (fontHeight / 2) : uvWidth;
				pVertexArray[index0].Position.x = posInPixels.x + offsetX;
				pVertexArray[index0].Position.y = posInPixels.y;
				pVertexArray[index0].Position.z = 0;
				//pVertexArray[index0].Position.w = 1;
				pVertexArray[index0].Color = upperColor;
				pVertexArray[index0].TexCoord.x = uvLeft;
				pVertexArray[index0].TexCoord.y = uvTop;
				const size_t index1 = i * 4 + 1;
				pVertexArray[index1].Position.x = posInPixels.x + offsetX + uvWidth;
				pVertexArray[index1].Position.y = posInPixels.y;
				pVertexArray[index1].Position.z = 0;
				//pVertexArray[index1].Position.w = 1;
				pVertexArray[index1].Color = upperColor;
				pVertexArray[index1].TexCoord.x = uvRight;
				pVertexArray[index1].TexCoord.y = uvTop;
				const size_t index2 = i * 4 + 2;
				pVertexArray[index2].Position.x = posInPixels.x + offsetX;
				pVertexArray[index2].Position.y = posInPixels.y + uvHeight;
				pVertexArray[index2].Position.z = 0;
				//pVertexArray[index2].Position.w = 1;
				pVertexArray[index2].Color = lowerColor;
				pVertexArray[index2].TexCoord.x = uvLeft;
				pVertexArray[index2].TexCoord.y = uvBottom;
				const size_t index3 = i * 4 + 3;
				pVertexArray[index3].Position.x = posInPixels.x + offsetX + uvWidth;
				pVertexArray[index3].Position.y = posInPixels.y + uvHeight;
				pVertexArray[index3].Position.z = 0;
				//pVertexArray[index3].Position.w = 1;
				pVertexArray[index3].Color = lowerColor;
				pVertexArray[index3].TexCoord.x = uvRight;
				pVertexArray[index3].TexCoord.y = uvBottom;

				// ボールド体の場合はもう少しオフセットしたほうがいいかも。
				// フォントによっては、強制的に半角幅分送るだけ、というのは問題あり。
				// 特にプロポーショナル フォントは英数字であっても文字幅が異なる。
				// モノスペースであっても、半角幅分とはかぎらない。
				offsetX += feed + 1;
			}
		}
		if (m_stringLength > 0)
		{
			return this->ReplaceVertexData(&m_vertexArray[0]);
		}
		else
		{
			return true;
		}
	}

} // end of namespace
