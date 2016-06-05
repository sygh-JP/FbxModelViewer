// Only UTF-8 or ASCII is available.

/////////////////////////////////////////////////////////////////////
// レンダリング ステート。
// サンプラーステートと違い、リフレクションを切った Compact Effects11 であってもこれはフル動作する模様。

// http://msdn.microsoft.com/ja-jp/library/ee416550.aspx

#if 0
RasterizerState RS_WireFrame
{
	FillMode = WIREFRAME;
	// TODO: MSAA の設定も必要。そのため、デバイス能力を把握している C++ 側で設定した方がいいかも。
};

RasterizerState RS_Solid
{
	FillMode = SOLID;
};
#endif

// HACK: メッシュ属性のブレンディング プロパティに応じて、アルファ ブレンドの ON/OFF は CPU 側で制御したほうがいいかも。
// アルファ ブレンドは結構重い処理で、フィルレートを食う。


// 何もしないブレンディング ステート。
BlendState BS_NoBlending
{
	BlendEnable[0] = FALSE;
	SrcBlend = ONE;
	DestBlend = ZERO;
	BlendOp = ADD;
	SrcBlendAlpha = ONE;
	DestBlendAlpha = ZERO;
	BlendOpAlpha = ADD;
	RenderTargetWriteMask[0] = 0x0F;
};

// 通常の線形合成のブレンディング ステート。
BlendState BS_NormalAlphaBlendingOn
{
	BlendEnable[0] = TRUE;
	SrcBlend = SRC_ALPHA;
	DestBlend = INV_SRC_ALPHA;
	BlendOp = ADD;
	//SrcBlendAlpha = ZERO;
	DestBlendAlpha = ZERO;
	SrcBlendAlpha = ONE;
	//DestBlendAlpha = ONE;
	BlendOpAlpha = ADD;
	RenderTargetWriteMask[0] = 0x0F;
};

// 加算合成のブレンディング ステート。
BlendState BS_SrcAlphaBlendingAdd
{
	BlendEnable[0] = TRUE;
	SrcBlend = SRC_ALPHA;
	DestBlend = ONE;
	BlendOp = ADD;
	SrcBlendAlpha = ZERO;
	DestBlendAlpha = ZERO;
	BlendOpAlpha = ADD;
	RenderTargetWriteMask[0] = 0x0F; // 全ビット ON（D3D10_COLOR_WRITE_ENABLE, D3D11_COLOR_WRITE_ENABLE）。
};


// 深度ステンシル ステートはエフェクト ファイルで管理したほうが都合がよいことが多い。

DepthStencilState DSS_DefaultDepthStencil
{
	DepthEnable = TRUE;
	DepthWriteMask = ALL; // 深度バッファへの書き込みを ON。
	DepthFunc = LESS; // デフォルト。
};

// 深度バッファへの書き込みを行なわない深度ステンシル ステート。ただし深度バッファの参照は行なう。
DepthStencilState DSS_DontWriteDepth
{
	DepthEnable = TRUE;
	DepthWriteMask = ZERO; // 深度バッファへの書き込みを OFF。
	DepthFunc = LESS; // デフォルト。
	//DepthFunc = ALWAYS; // 深度比較は常に合格。
};

DepthStencilState DSS_NoDepthStencil
{
	DepthEnable = FALSE;
	DepthWriteMask = ZERO; // 深度バッファへの書き込みを OFF。
};
