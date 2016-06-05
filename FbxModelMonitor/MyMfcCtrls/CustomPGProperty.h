#pragma once

#include "../MyWpfGraphLibMfc/Wrappers/GradientEditorWrapper.hpp"
#include "MyMath.hpp"


// カスタムの CMFCPropertyGridProperty 派生クラスを定義する。
// CMFCPropertyGridProperty 自体はビジュアルなコントロールではなく、
// CMFCPropertyGridCtrl に表示される子コントロールを管理するための Proxy クラス。
// つまり、メッセージ ハンドラーの直接定義ではなく、
// カスタマイズ ポイントである代理の仮想メソッドをオーバーライドすることで動作をカスタマイズする方式になっている。


class CMyReadonlyPGProperty : public CMFCPropertyGridProperty
{
public:
	CMyReadonlyPGProperty(const CString& strName, const _variant_t& inValue, LPCTSTR lpszDescr = nullptr, DWORD_PTR dwData = 0);
	virtual CWnd* CreateInPlaceEdit(CRect rectEdit, BOOL& bDefaultFormat) override;
};


// 文字列と int 型の値のペアをドロップダウン リストで保持するプロパティ。
// 内部 Variant として保持される値は int となる。
// ソートはせず、順序を維持する。
// 
// CMFCPropertyGridProperty には AddOption(), GetOption() メソッドは存在するが、
// 現在ドロップダウン リストの何番目の項目が選択されているのかを直接取得するメソッド（CComboBox::GetCurSel() 相当）、
// および指定インデックスの項目を直接選択するメソッド（CComboBox::SetCurSel() 相当）は存在しない模様。
class CMyIntDropDownListPGProperty : public CMyReadonlyPGProperty
{
public:
	typedef std::pair<CString, int> TDropDownValue;
	typedef std::vector<TDropDownValue> TDropDownValuesArray;

public:
	CMyIntDropDownListPGProperty(const CString& strName, int value, LPCTSTR lpszDescr = nullptr, DWORD_PTR dwData = 0);
	virtual CString FormatProperty() override;
	//virtual CWnd* CreateInPlaceEdit(CRect rectEdit, BOOL& bDefaultFormat) override;

	// Visual C++ 2013 以降（C++11）であれば std::vector のコンストラクタに初期化子リスト構文も使えるので活用すること。
	void SetDropDownValues(const TDropDownValuesArray& ddv);

	// 他の型を受け付けないようにするため、基底クラスのメソッド GetValue(), SetValue() と同じ名前のメソッドを定義して、
	// あえて隠ぺいしようと思ったが、ともに仮想メソッドなので派生クラスでオーバーロードを定義できない。

	int GetCurrentValue() const
	{
		_ASSERTE(m_varValue.vt == VT_INT);
		return m_varValue.intVal;
	}

	void SetCurrentValue(int value)
	{
		this->SetValue(_variant_t(value));
	}

protected:
	virtual BOOL TextToVar(const CString& strText) override;
private:
	TDropDownValuesArray m_dropDownValues;
};


// エディットコントロールに上下限外の値を入力したときに自動クランプするプロパティ。（int 専用、スピンボタン付き）
class CMyIntSpinPGProperty : public CMFCPropertyGridProperty
{
public:
	CMyIntSpinPGProperty(const CString& strName, int value, int minVal, int maxVal, LPCTSTR lpszDescr = nullptr, DWORD_PTR dwData = 0);
	virtual BOOL OnUpdateValue() override;

	int GetMinValue() const { return m_nMinValue; }
	int GetMaxValue() const { return m_nMaxValue; }
	int GetCurrentValue() const
	{
		_ASSERTE(m_varValue.vt == VT_INT);
		return m_varValue.intVal;
	}

protected:
	virtual BOOL TextToVar(const CString& strText) override;
};


// エディットコントロールに上下限外の値を入力したときに自動クランプするプロパティ。（double 専用、スピンボタン付き）
class CMyDoubleSpinPGProperty : public CMFCPropertyGridProperty
{
public:
	CMyDoubleSpinPGProperty(const CString& strName, double value, double minVal, double maxVal, double delta, LPCTSTR lpszDescr = nullptr, DWORD_PTR dwData = 0);
	virtual CSpinButtonCtrl* CreateSpinControl(CRect rectSpin) override;
	virtual CWnd* CreateInPlaceEdit(CRect rectEdit, BOOL& bDefaultFormat) override;
	virtual BOOL OnUpdateValue() override;

	double GetMinValue() const { return m_minVal; }
	double GetMaxValue() const { return m_maxVal; }
	double GetDelta() const { return m_delta; }
	double GetCurrentValue() const
	{
		_ASSERTE(m_varValue.vt == VT_R8);
		return m_varValue.dblVal;
	}
	void SetCurrentValue(double val, bool notifiesPropertyChanged = false);
	void IncreaseValue(bool notifiesPropertyChanged = false);
	void DecreaseValue(bool notifiesPropertyChanged = false);
#if 0
	// スピン コントロールの上下ボタンによる変更の際に AFX_WM_PROPERTY_CHANGED で通知されるためには、
	// 構築後にあらかじめ呼び出してバインドしておく必要がある。
	void SetManagerCtrl(CMFCPropertyGridCtrl* pManagerCtrl) { m_pManagerCtrl = pManagerCtrl; }
#endif

private:
	const double m_minVal;
	const double m_maxVal;
	const double m_delta;
#if 0
private:
	CMFCPropertyGridCtrl* m_pManagerCtrl;
#endif
protected:
	virtual BOOL TextToVar(const CString& strText) override;
};


#if 0
// C++ 用の GDI+ では .NET と違って LinearGradientBrush::SetInterpolationColors() がオフセットとカラーを別々の配列で受け取るため、
// パッケージ化したクラス／構造体の配列で管理しないほうが返ってレンダリング時の効率が良くなる。
class MyGdipGradientColorStop final
{
public:
	float Offset; // 0.0～1.0 の範囲。
	Gdiplus::Color Color;
public:
	MyGdipGradientColorStop()
		: Offset()
		, Color(Gdiplus::Color::White)
	{}
	MyGdipGradientColorStop(float offset, Gdiplus::Color color)
		: Offset(offset)
		, Color(color)
	{}
};
#endif


// グラデーション情報を管理するデータ形式として、
// シリアライズ・逆シリアライズ用のテキスト データ、
// レンダリング用の GDI+ グラデーション ストップ データ、
// WPF グラデーション エディター用のデータが存在するが、このクラスのインスタンスで直接管理するのはテキストと GDI+ の2つの情報。
class CMyGradientStopsProperty : public CMFCPropertyGridProperty
{
private:
	// WPF グラデーション エディター用のシングルトン インスタンス。
	static MyWpfGraphLibWrapper::MiscControls::IMyGradientEditorWrapper::TSharedPtr m_gradEditor;

public:
	static void DestroyWpfGradientEditor()
	{
		// Close() は明示的に呼び出さなくても、オーナーが閉じられるタイミングでも自動的に実行される。
		// その際、なぜか RPC call cancelled の Win32 例外 0x0000071A が発生する。
		// デバッグ モードが「混合」になっていないと、KernelBase.dll で発生していると報告される。
		// 「混合」にすると、EXE 側で例外が発生していると報告される。
		// どうも WPF ウィンドウの Win32 オーナーウィンドウを DestroyWindow() して連鎖的に Close() されるときや、
		// 明示的に Close() するときなどに例外が発生している模様。
		// ネット上の情報を見る限り、Unhandled Exception ではないので気にする必要はない、とのことだが……

		// Close() はネイティブ ラッパーのインスタンスが破棄される際にデストラクタで確実に実行される。
		// また、イベント リスナーはデストラクタによってアンバインドされる。
		m_gradEditor.reset();
	}

private:
	std::vector<float> m_gradStopOffsets;
	std::vector<Gdiplus::Color> m_gradStopColors;
private:
	CStringW m_strPrevCachedValue;
public:
	void UpdateCachedValue()
	{ m_strPrevCachedValue = m_varValue.bstrVal; }
public:
	bool GetIsCurrentValueSameAsPrevCachedValue() const
	{ return (m_varValue.bstrVal != nullptr) && m_strPrevCachedValue.Compare(m_varValue.bstrVal) == 0; }
public:
	CStringW GetCurrentValue() const
	{
		_ASSERTE(m_varValue.vt == VT_BSTR);
		return m_varValue.bstrVal;
	}
public:
	size_t GetGradientColorStopCount() const
	{
		_ASSERTE(m_gradStopOffsets.size() == m_gradStopColors.size());
		return m_gradStopOffsets.size();
	}
public:
	// WPF グラデーション エディターに設定するのに使う。
	void GetGradientColorStops(MyWpfGraphLibWrapper::MiscControls::TGradientColorParamsArray& gradColorArray) const
	{
		gradColorArray.clear();
		_ASSERTE(m_gradStopOffsets.size() == m_gradStopColors.size());
		// グラデーション ストップの左右両端は無視する。
		for (size_t i = 1; i + 1 < m_gradStopOffsets.size(); ++i)
		{
			gradColorArray.push_back(MyWpfGraphLibWrapper::MiscControls::GradientColorParams(m_gradStopOffsets[i],
				m_gradStopColors[i].ToCOLORREF()
				));
		}
	}
public:
	// グラデーション テクスチャ作成用のバイナリ情報として使う。
	void GetGradientColorStops(MyMath::TMyGradientColorStopArray& gradColorArray) const
	{
		gradColorArray.clear();
		_ASSERTE(m_gradStopOffsets.size() == m_gradStopColors.size());
#if 0
		// グラデーション ストップの左右両端も考慮する。
		for (size_t i = 0; i < m_gradStopOffsets.size(); ++i)
#else
		// グラデーション ストップの左右両端は無視する。
		for (size_t i = 1; i + 1 < m_gradStopOffsets.size(); ++i)
#endif
		{
			gradColorArray.push_back(MyMath::MyGradientColorStop(m_gradStopOffsets[i],
				MyMath::ColorRgba(m_gradStopColors[i].GetR(), m_gradStopColors[i].GetG(), m_gradStopColors[i].GetB())
				));
		}
	}
public:
	void SetGradientColorStops(const MyMath::TMyGradientColorStopArray& gradColorArray);
private:
	bool ScanGradientString(const CStringW& strTarget);
public:
	CMyGradientStopsProperty(const CString& strName, const CStringW& iniValue, LPCTSTR lpszDescr = nullptr, DWORD_PTR dwData = 0)
		: CMFCPropertyGridProperty(strName, _variant_t(iniValue), lpszDescr, dwData)
	{}

	// デフォルトで [...] ボタンが表示されるようになる。
	virtual BOOL HasButton() const override
	{ return true; }

	virtual BOOL OnUpdateValue() override;

	virtual void OnClickButton(CPoint point) override;

	virtual void OnDrawValue(CDC* pDC, CRect rect) override;

	virtual CString GetNameTooltip() override
	{
		// リスト左側の名前部分をポイントすると出現するツールチップに表示されるテキストを指定する。
		return _T("Format = \"Offset0 (R0 G0 B0); Offset1 (R1 G1 B1);...\"");
	}

#if 0
	virtual void OnDestroyWindow() override
	{
		ATLTRACE(__FUNCTIONW__ L"() Before\n");
		__super::OnDestroyWindow();
		ATLTRACE(__FUNCTIONW__ L"() After\n");
	}

	virtual BOOL OnEdit(LPPOINT lptClick) override
	{
		ATLTRACE(__FUNCTIONW__ L"() Before\n");
		BOOL retval = __super::OnEdit(lptClick);
		ATLTRACE(__FUNCTIONW__ L"() After\n");
		return retval;
	}

	virtual BOOL OnEndEdit() override
	{
		// この時点ですでに AFX_WM_PROPERTY_CHANGED が発行されてしまっている。
		ATLTRACE(__FUNCTIONW__ L"() Before\n");
		BOOL retval = __super::OnEndEdit();
		ATLTRACE(__FUNCTIONW__ L"() After\n");
		return retval;
	}

	virtual BOOL IsValueChanged() const override
	{
		// この時点ですでに m_varValue は書き換わっている。
		ATLTRACE(__FUNCTIONW__ L"() Before = \"%s\"\n", m_varValue.bstrVal);
		BOOL retval = __super::IsValueChanged();
		ATLTRACE(__FUNCTIONW__ L"() After = \"%s\"\n", m_varValue.bstrVal);
		// 変更があった場合にはテキストがボールド体になるが、IsValueChanged() はそれを決めるためだけのものらしい？
		return retval;
	}

	virtual HBRUSH OnCtlColor(CDC* pDC, UINT nCtlColor) override
	{
		return static_cast<HBRUSH>(::GetStockObject(HOLLOW_BRUSH));
	}
#endif
};


#if 0
class CMyPropertyGridCtrl : public CMFCPropertyGridCtrl
{
	virtual int OnDrawProperty(CDC* pDC, CMFCPropertyGridProperty* pProp) const override
	{
		return __super::OnDrawProperty(pDC, pProp);
	}
};
#endif
