
#include "stdafx.h"

#include "CustomPGProperty.h"

#include "DebugNew.h"
//#include "MyMath.hpp"
#include "MyUtil.h"


// VARIANT ラッパークラスはいくつかあるが、COleVariant は MFC で使用可能。
// ATL::CComVariant は MFC/ATL で使用可能。
// _variant_t は全エディションの MSVC で使用可能（CRT というか C++ 向け Win32 ヘルパーらしい）。
// COleVariant は int や bool を引数にとるコンストラクタをサポートしない代わりに、CByteArray や CLongBinary などを受け取ることができる。
// CComVariant は int を受け取るコンストラクタを持つが、デフォルトで VT_INT でなく VT_I4 になるので注意。
// VT_INT や VT_BOOL を気軽にかつ確実に使いたい場合は _variant_t を使ったほうが楽。
// ちなみに VT_I4 は long 用であり、VT_INT とは異なるので注意。
// COleVariant, CComVariant, _variant_t はいずれも VARIANT 構造体の public 派生で、
// さらに VARIANT への参照もしくはポインタを受け取るコンストラクタを持つので、相互に暗黙変換できる。
// なお、BSTR ラッパーである ATL::CComBSTR は使い方が難しく、_bstr_t を使ったほうがよいらしい。


#pragma region // CMyReadonlyPGProperty //

// NOTE: CMFCPropertyGridProperty::AllowEdit() で読み取り専用にしたらコンテキスト メニューが出なくなるだけでなく、
// Ctrl キーを押すときにフォーカスが外れるらしく、クリップボードに名前をコピーすることすらできなくなる。
// 代替機能を用意する。

CMyReadonlyPGProperty::CMyReadonlyPGProperty(const CString& strName, const _variant_t& value, LPCTSTR lpszDescr, DWORD_PTR dwData)
	: CMFCPropertyGridProperty(strName, _variant_t(value), lpszDescr, dwData)
{
}

CWnd* CMyReadonlyPGProperty::CreateInPlaceEdit(CRect rectEdit, BOOL& bDefaultFormat)
{
	// 読み取り専用の CEdit を返す。
	auto* pWndEdit = new CEdit();
	const DWORD dwStyle = WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | ES_READONLY;
	pWndEdit->Create(dwStyle, rectEdit, m_pWndList, AFX_PROPLIST_ID_INPLACE);
	pWndEdit->SetWindowText(this->FormatProperty());
	bDefaultFormat = false;
	return pWndEdit;
}

#pragma endregion


#pragma region // CMyIntDropDownListPGProperty //

void CMyIntDropDownListPGProperty::SetDropDownValues(const TDropDownValuesArray& ddv)
{
	m_dropDownValues = ddv;
	this->RemoveAllOptions();
	for (TDropDownValuesArray::const_iterator it = m_dropDownValues.begin(); it != m_dropDownValues.end(); ++it)
	{
		this->AddOption(it->first);
	}
}

// 数値の代わりに対応する文字列を表示させる。
CString CMyIntDropDownListPGProperty::FormatProperty()
{
	const int val = this->GetCurrentValue();
	for (TDropDownValuesArray::const_iterator it = m_dropDownValues.begin(); it != m_dropDownValues.end(); ++it)
	{
		if (val == it->second)
		{
			return it->first;
		}
	}
	// リストにない場合は空文字列。
	return CString();
}

#if 0
CWnd* CMyIntDropDownListPGProperty::CreateInPlaceEdit(CRect rectEdit, BOOL& bDefaultFormat)
{
	// 読み取り専用の CEdit を返す。
	auto* pWndEdit = new CEdit();
	const DWORD dwStyle = WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | ES_READONLY;
	pWndEdit->Create(dwStyle, rectEdit, m_pWndList, AFX_PROPLIST_ID_INPLACE);
	pWndEdit->SetWindowText(this->FormatProperty());
	bDefaultFormat = false;
	return pWndEdit;
}
#endif

CMyIntDropDownListPGProperty::CMyIntDropDownListPGProperty(const CString& strName, int value, LPCTSTR lpszDescr, DWORD_PTR dwData)
	: CMyReadonlyPGProperty(strName, _variant_t(value), lpszDescr, dwData)
{
	// CEdit にフォーカスがあるときの表示を通常と同じにする。
	//this->AllowEdit(false);
}

// MSDN でドキュメント化されてない仮想メソッド。
// コンボボックスで文字列を選択したときに、内部 Variant プロパティに設定する値に変換する。
BOOL CMyIntDropDownListPGProperty::TextToVar(const CString& strText)
{
	for (TDropDownValuesArray::const_iterator it = m_dropDownValues.begin(); it != m_dropDownValues.end(); ++it)
	{
		if (strText == it->first)
		{
			m_varValue = _variant_t(it->second);
			return true;
		}
	}
	return false;
}

#pragma endregion




#pragma region // CMyIntSpinPGProperty //

// HACK: CMyIntSpinPGProperty も CMyDoubleSpinPGProperty 同様にステップ幅を指定できるようにする。

CMyIntSpinPGProperty::CMyIntSpinPGProperty(const CString& strName, int value, int minVal, int maxVal, LPCTSTR lpszDescr, DWORD_PTR dwData)
	: CMFCPropertyGridProperty(strName, _variant_t(value), lpszDescr, dwData)
{
	ASSERT(minVal <= value && value <= maxVal);
	this->EnableSpinControl(true, minVal, maxVal);
}

BOOL CMyIntSpinPGProperty::OnUpdateValue()
{
	const BOOL ret = __super::OnUpdateValue();
	if (ret)
	{
		m_varValue = _variant_t(MyUtils::Clamp(this->GetCurrentValue(), this->GetMinValue(), this->GetMaxValue()));
	}

	return ret;
}

BOOL CMyIntSpinPGProperty::TextToVar(const CString& strText)
{
	const int val = ::StrToInt(static_cast<LPCTSTR>(strText));
	m_varValue = _variant_t(val);
	return true;
}

#pragma endregion




#pragma region // CMyDoubleSpinButtonCtrl //

class CMyDoubleSpinButtonCtrl : public CMFCSpinButtonCtrl
{
public:
#if 1
	explicit CMyDoubleSpinButtonCtrl(CMyDoubleSpinPGProperty* pProp)
		: m_pProp(pProp)
	{}
#else
	CMyDoubleSpinButtonCtrl(double minVal, double maxVal, double delta)
		: m_minVal(minVal)
		, m_maxVal(maxVal)
		, m_delta(delta)
	{}
#endif

private:
#if 1
	// 上下限値を直接保持せず、CMyDoubleSpinPGProperty のプロパティと連動させる。
	CMyDoubleSpinPGProperty* m_pProp;
#else
	const double m_minVal;
	const double m_maxVal;
	const double m_delta;
#endif

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnDeltapos(NMHDR *pNMHDR, LRESULT *pResult);
};

BEGIN_MESSAGE_MAP(CMyDoubleSpinButtonCtrl, CMFCSpinButtonCtrl)
	ON_NOTIFY_REFLECT(UDN_DELTAPOS, &CMyDoubleSpinButtonCtrl::OnDeltapos)
END_MESSAGE_MAP()


void CMyDoubleSpinButtonCtrl::OnDeltapos(NMHDR *pNMHDR, LRESULT *pResult)
{
	auto pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	// TODO: ここにコントロール通知ハンドラ コードを追加します。

	TRACE(__FUNCTIONW__ L"()\n");

	// NOTE: CMFCPropertyGridProperty::SetValue(const _variant_t&) を直接呼び出すと、
	// "free heap block ? modified at ? after it was freed" のヒープエラーが発生する。
	// したがって、CMFCPropertyGridProperty::m_varValue および
	// CMFCPropertyGridProperty::m_pWndInPlace のテキストを直接書き換えるメンバ関数を用意する。
	if (m_pProp)
	{
		if (pNMUpDown->iDelta > 0)
		{
			m_pProp->DecreaseValue(true);
		}
		else if (pNMUpDown->iDelta < 0)
		{
			m_pProp->IncreaseValue(true);
		}
		//m_pProp->OnUpdateValue();
	}

	*pResult = 0;
}

#pragma endregion


#pragma region // CMyDoubleSpinPGProperty //

#ifndef AFX_PROP_HAS_SPIN
// 公開されていないので、MFC 9.0 の実装を参考に定義。
#define AFX_PROP_HAS_SPIN 0x0004
#endif

CMyDoubleSpinPGProperty::CMyDoubleSpinPGProperty(const CString& strName, double value, double minVal, double maxVal, double delta, LPCTSTR lpszDescr, DWORD_PTR dwData)
	: CMFCPropertyGridProperty(strName, _variant_t(value), lpszDescr, dwData)
	, m_minVal(minVal)
	, m_maxVal(maxVal)
	, m_delta(delta)
#if 0
	, m_pManagerCtrl(nullptr)
#endif
{
	ASSERT(minVal <= value && value <= maxVal);

	m_dwFlags |= AFX_PROP_HAS_SPIN;
	// CMFCPropertyGridProperty コンストラクタの第2引数に整数以外の型を渡した後で EnableSpinControl() を呼び出すと、
	// EnableSpinControl() 内の ASSERT() に引っかかる (afxpropertygridctrl.cpp) ので、直接スタイルを設定する。
}

BOOL CMyDoubleSpinPGProperty::OnUpdateValue()
{
	const BOOL ret = __super::OnUpdateValue();
	if (ret)
	{
		m_varValue = _variant_t(MyUtils::Clamp(this->GetCurrentValue(), this->GetMinValue(), this->GetMaxValue()));
	}

	return ret;
}

CSpinButtonCtrl* CMyDoubleSpinPGProperty::CreateSpinControl(CRect rectSpin)
{
	auto* pSpin = new CMyDoubleSpinButtonCtrl(this);
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_ALIGNRIGHT;

	pSpin->Create(dwStyle, rectSpin, m_pWndList, AFX_PROPLIST_ID_INPLACE);
	return pSpin;
}

CWnd* CMyDoubleSpinPGProperty::CreateInPlaceEdit(CRect rectEdit, BOOL& bDefaultFormat)
{
	auto pOutWnd = __super::CreateInPlaceEdit(rectEdit, bDefaultFormat);
#if 0
	auto pEdit = dynamic_cast<CEdit*>(pOutWnd);
	if (pEdit)
	{
		// 右寄せにする。
		pEdit->ModifyStyle(0, ES_RIGHT);
		// アクティブになったときにしかスピンが表示されないので、むしろ右寄せしないほうがいいかも……
		// HACK: AdjustInPlaceEditRect() のオーバーライドで対応できるか？
	}
#endif
	return pOutWnd;
}

BOOL CMyDoubleSpinPGProperty::TextToVar(const CString& strText)
{
	// 変換不能な場合はゼロ扱い。
	const double val = _tcstod(static_cast<LPCTSTR>(strText), nullptr);
	m_varValue = val;
	return true;
}

void CMyDoubleSpinPGProperty::SetCurrentValue(double val, bool notifiesPropertyChanged)
{
	//ATLTRACE("Pre-Set: VarValue = %.16f\n", this->GetCurrentValue());
	m_varValue = val;
	__super::SetModifiedFlag();
	// NOTE: 実数型の場合は浮動小数誤差があることに注意。オリジナルと同じ値とするには、全ビットが同じでなければならない。
	// テキスト文字列を経由して10進数でシリアライズする場合は必ず誤差が発生する。
	//ATLTRACE("Post-Set: VarValue = %.16f\n", this->GetCurrentValue());
	if (m_pWndInPlace) // プロパティ編集中は有効だが、編集されていない場合は無効らしい。編集開始のたびに生成され、編集終了のたびに削除されるらしい？
	{
		CString temp;
		temp.Format(_T("%f"), val);
		m_pWndInPlace->SetWindowText(temp);
	}
	else
	{
		// 編集中でない場合は明示的に再描画する必要がある。
		this->Redraw();
	}
	// 外部からユーザーコードで CMFCPropertyGridCtrl へのポインタを与える必要はなく、CMFCPropertyGridProperty::m_pWndList を直接使えばいいらしい。
#if 0
	if (m_pManagerCtrl && notifiesPropertyChanged)
	{
		ASSERT_VALID(m_pManagerCtrl);
		// AFX_WM_PROPERTY_CHANGED を送信。
		m_pManagerCtrl->OnPropertyChanged(this);
	}
#else
	if (m_pWndList && notifiesPropertyChanged)
	{
		ASSERT_VALID(m_pWndList);
		// AFX_WM_PROPERTY_CHANGED を送信。
		m_pWndList->OnPropertyChanged(this);
	}
#endif
}

void CMyDoubleSpinPGProperty::IncreaseValue(bool notifiesPropertyChanged)
{
	const double newVal = MyUtils::Clamp(this->GetCurrentValue() + this->GetDelta(), this->GetMinValue(), this->GetMaxValue());
	this->SetCurrentValue(newVal, notifiesPropertyChanged);
}

void CMyDoubleSpinPGProperty::DecreaseValue(bool notifiesPropertyChanged)
{
	const double newVal = MyUtils::Clamp(this->GetCurrentValue() - this->GetDelta(), this->GetMinValue(), this->GetMaxValue());
	this->SetCurrentValue(newVal, notifiesPropertyChanged);
}

#pragma endregion


#pragma region // CMyGradientStopsProperty //

MyWpfGraphLibWrapper::MiscControls::IMyGradientEditorWrapper::TSharedPtr CMyGradientStopsProperty::m_gradEditor;

namespace
{
	inline Gdiplus::Color ToGdipColorRgb(MyMath::ColorRgba color)
	{
		return Gdiplus::Color(color.R, color.G, color.B);
	}

	inline Gdiplus::Color ToGdipColorRgb(int r, int g, int b)
	{
		const byte cR = byte(MyUtils::Clamp<int>(r, 0, 0xFF));
		const byte cG = byte(MyUtils::Clamp<int>(g, 0, 0xFF));
		const byte cB = byte(MyUtils::Clamp<int>(b, 0, 0xFF));
		return Gdiplus::Color(cR, cG, cB);
	}
}

bool CMyGradientStopsProperty::ScanGradientString(const CStringW& strTarget)
{
	m_gradStopOffsets.clear();
	m_gradStopColors.clear();
	std::vector<float> gradStopOffsets;
	std::vector<Gdiplus::Color> gradStopColors;
	const LPCWSTR sepChars = L";";
	int tokenStart = 0;
	CStringW strRes = strTarget.Tokenize(sepChars, tokenStart);
	while (!strRes.IsEmpty())
	{
		// 文字列を解析して、グラデーション ストップの情報として分解する。
		// フォーマット書式に関してはオーバーライドした GetNameTooltip() メソッドを参照のこと。
		float offset = 0;
		int r = 0, g = 0, b = 0;
		const int scanCount = swscanf_s(strRes, L" %f ( %d %d %d ) ", &offset, &r, &g, &b);
		if (scanCount != 4)
		{
			return false;
		}
		gradStopOffsets.push_back(MyUtils::Clamp<float>(offset * 0.01f, 0.0f, 1.0f));
		gradStopColors.push_back(ToGdipColorRgb(r, g, b));

		strRes = strTarget.Tokenize(sepChars, tokenStart);
		if (tokenStart == -1) // 区切りであるセミコロンがない場合。
		{
			break;
		}
	};
	// GDI+ では WPF/Photoshop と違い、グラデーションの最初と最後にストップを追加してやる必要がある。
	// グラデーション データをテクスチャのソースとして DIB 化するときも同様の処理が要る。
	// なお、GDI+ レンダリング時に毎回配列の先頭と末尾にストップを追加した一時データを生成することはせず、
	// レンダリング専用データを事前作成しておく。
	if (!gradStopOffsets.empty())
	{
		gradStopOffsets.insert(gradStopOffsets.begin(), 1, 0.0f);
		gradStopOffsets.insert(gradStopOffsets.end(), 1, 1.0f);
	}
	if (!gradStopColors.empty())
	{
		gradStopColors.insert(gradStopColors.begin(), 1, *gradStopColors.begin());
		gradStopColors.insert(gradStopColors.end(), 1, *(--gradStopColors.end()));
		// end イテレータの1つ前が最終要素。
	}

	gradStopOffsets.swap(m_gradStopOffsets);
	gradStopColors.swap(m_gradStopColors);
	return true;
}

void CMyGradientStopsProperty::SetGradientColorStops(const MyMath::TMyGradientColorStopArray& gradColorArray)
{
	m_gradStopOffsets.clear();
	m_gradStopColors.clear();
	// グラデーション ストップの左右両端も考慮する。
	if (!gradColorArray.empty())
	{
		const auto& grad = *gradColorArray.begin();
		m_gradStopOffsets.push_back(0.0f);
		m_gradStopColors.push_back(ToGdipColorRgb(grad.Color));
	}
	for (const auto& grad : gradColorArray)
	{
		m_gradStopOffsets.push_back(grad.Offset);
		m_gradStopColors.push_back(ToGdipColorRgb(grad.Color));
	}
	if (!gradColorArray.empty())
	{
		const auto& grad = *(--gradColorArray.end());
		m_gradStopOffsets.push_back(1.0f);
		m_gradStopColors.push_back(ToGdipColorRgb(grad.Color));
	}
	_ASSERTE(m_gradStopOffsets.size() == m_gradStopColors.size());
	// グラデーション文字列の変更。
	CStringW strSerialized;
#if 0
	// グラデーション ストップの左右両端は無視する。
	for (size_t i = 1; i + 1 < gradColorArray.size(); ++i)
#else
	for (size_t i = 0; i < gradColorArray.size(); ++i)
#endif
	{
		const auto grad = gradColorArray[i];
		strSerialized.AppendFormat(L"%.1f(%d %d %d);", grad.Offset * 100.0f, grad.Color.R, grad.Color.G, grad.Color.B);
	}
	m_varValue = strSerialized;
	this->Redraw();
}

BOOL CMyGradientStopsProperty::OnUpdateValue()
{
	// エディットボックスにフォーカス イン・フォーカス アウトするだけでコールバックされる。

	// この時点でまだ m_varValue は書き換わっていない。
	ATLTRACE(__FUNCTIONW__ L"() Before = \"%s\"\n", m_varValue.bstrVal);
	if (m_pWndInPlace)
	{
		// AFX_WM_PROPERTY_CHANGED が発行される前に書式をチェックして、不適合値を編集開始時の値に戻すならばこのタイミング。
		// 基底クラスの CMFCPropertyGridProperty::OnUpdateValue() を呼び出さずに直接 true を返すと、Esc キーを押下したのと同じ効果になる模様。
		// 直接 false を返すと、Enter を押してもエディットボックスが閉じなくなる。
		// そもそも CMFCPropertyGridCtrl::ValidateItemData() をオーバーライドするべきなのか？
		// 入力されたテキストはたとえ不正フォーマットであってもそのままにしておいて、グラデーションのみを無効化して赤字にしてしまうとよいかも。
		CString strWndText;
		m_pWndInPlace->GetWindowText(strWndText);
		// 前回の設定値とまったく同じ場合は文字列スキャンおよび AFX_WM_PROPERTY_CHANGED による通知の必要はない。
		// なお、16進数で数値入力ができるように機能追加する場合でも、大文字・小文字の区別はする。
		if ((m_varValue.bstrVal != nullptr) && strWndText.Compare(m_varValue.bstrVal) != 0)
		{
			//ATLTRACE(_T(__FUNCTION__) _T("() WinText = %s\n"), strWndText.GetString());
			const bool scanResult = this->ScanGradientString(strWndText);
			if (!scanResult)
			{
				// false を返すと Enter を押しても閉じなくなる。true を返すと設定がキャンセルされる。
				// なお、ここでエラー通知のためのメッセージボックスの表示はしないほうがよい（複数回表示されてしまう）。
				//return true;
			}
		}
		else
		{
			ATLTRACE(__FUNCTIONW__ L"() Cancelled.\n");
			return true;
		}
	}
	const BOOL retval = __super::OnUpdateValue();
	// 前回の設定値と異なる場合、
	// この時点ですでに AFX_WM_PROPERTY_CHANGED が発行されてしまっている。
	ATLTRACE(__FUNCTIONW__ L"() After = \"%s\"\n", m_varValue.bstrVal);
	return retval;
}

void CMyGradientStopsProperty::OnClickButton(CPoint point)
{
	//AfxMessageBox(_T("Button Pressed!!"));

	// WPF のモードレス ダイアログを表示中、MFC のモーダル メッセージ ボックス（もしくはタスク ダイアログ）を出すと、
	// 前後関係が崩れてしまう。モーダル ダイアログとして使用したほうがよさげ。
	// 他にも、ホスト ウィンドウを MFC 側で用意して、ソコに WPF コントロールを埋め込んでモードレス化する方法もあるが、
	// こちらはキーボード ナビゲーションなどかなり事前設定するべきことが多く、難易度が高い。
	if (!m_gradEditor)
	{
		// Win32/MFC アプリから WPF コントロールを初回表示（コールド スタート）する際、
		// というか最初に C++/CLI コードを実行するタイミングで CLR をロードして JIT コンパイルするのに異様に時間がかかるときがある。
		// 一度マネージ コードを実行してしまえば次回からは JIT キャッシュが効くので高速だが、
		// ビルドし直したり Windows を再起動したりするとコールド スタートになる。
		// ビルドし直すことによる影響は比較的小さいらしいので、
		// 共通言語ランタイム（基本クラス ライブラリを含む）のほうの初回ロードに異様に時間がかかっている可能性が高い。
		// 同じ .NET でも C# のような純粋なマネージ言語のみで書かれたアプリは、たとえ同規模であってもここまで起動が遅いことはない。
		// C++/CLI のコンソール アプリが C# のそれと比べて異様に起動に時間がかかるのも、同様の問題が潜在していることに起因しているように思われる。
		const CWaitCursor waitCursor;
		m_gradEditor = MyWpfGraphLibWrapper::MiscControls::IMyGradientEditorWrapper::Create(AfxGetMainWnd()->GetSafeHwnd(), 0, 0, 0, 0, L"MyWpfGradientEditor");
		// 簡単のため、メインフレーム ウィンドウをオーナーにしてモーダル ダイアログを表示している。
		// ドッキング ペインをフローティングしてメインフレームから切り離した状態でダイアログを表示しても、
		// 前後関係はちゃんと正しく動作するので問題ない。
		// TODO: もしカスタマイズしたければ外部から依存関係を注入する必要がある。

		// TODO: イベント リスナーのバインド。モードレスでの運用はせず、常にモーダルなので必要ない？

		// TODO: アプリケーション設定（Photoshop のようなグラデーション プリセット）の読み込み。
		// Gradients.psp ファイルが読み込めるとなおよい。
	}

	// 現在のグラデーション プロパティ設定をもとに、ダイアログに事前設定しておく。
	{
		MyWpfGraphLibWrapper::MiscControls::TGradientColorParamsArray gradColorArray;
		this->GetGradientColorStops(gradColorArray);
		m_gradEditor->SetGradientColorStops(gradColorArray);
	}

	// グラデーション エディタ ダイアログを表示。
	if (m_gradEditor->ShowModalDialog())
	{
		// [OK] が選択されたので、グラデーション情報をテキスト情報としてシリアライズ（フォーマット）し、
		// プロパティ エディタに値を設定したのち、変更を通知する。
		// フォーマット書式に関してはオーバーライドした GetNameTooltip() メソッドを参照のこと。

		MyWpfGraphLibWrapper::MiscControls::TGradientColorParamsArray gradColorArray;
		m_gradEditor->GetGradientColorStops(gradColorArray);
		CStringW strSerialized;
		for (auto grad : gradColorArray)
		{
			strSerialized.AppendFormat(L"%.1f(%d %d %d);", grad.Offset * 100.0f, GetRValue(grad.Color), GetGValue(grad.Color), GetBValue(grad.Color));
		}
		this->ScanGradientString(strSerialized);
		m_varValue = strSerialized;
		//__super::SetModifiedFlag();
#if 0
		if (m_pWndInPlace)
		{
			m_pWndInPlace->SetWindowText(strSerialized);
		}
#endif

#if 0
		if (m_pWndList)
		{
			ASSERT_VALID(m_pWndList);
			// AFX_WM_PROPERTY_CHANGED を送信。
			m_pWndList->OnPropertyChanged(this);
		}
#endif
		this->Redraw();

		// HACK: どうもエディタの [OK] ボタン押下後、AFX_WM_PROPERTY_CHANGED が複数回発行される現象が発生する？
		// 3回だったり2回だったり、よく分からない。
		// グラデーション テクスチャの書き換えは比較的重い処理なので、多重書き換えはきちんと防止したい。
		// 前回の設定値とまったく同じグラデーション情報文字列だった場合、書き換えは行なわない、というようにしたほうがよいかも。
		// 
		// --> CMyDoubleSpinPGProperty のときとは違って、CMFCPropertyGridCtrl::OnPropertyChanged() の明示的呼び出しは不要らしい。
		// 内部で m_varValue の古い値をキャッシュしているらしい？　SetModifiedFlag() 内で比較して、変更があったら
		// AFX_WM_PROPERTY_CHANGED を発行している？　SetModifiedFlag() は関係ない？　要調査。
		// なお、OnPropertyChanged() を呼び出さないようにしても、依然として複数発行される現象は残っている。
		// --> まずエディットボックスにフォーカスして[...]ボタンを押下すると現象が発生する模様。OnUpdateValue() も複数回コールバックされる。
		// エディットボックスにフォーカスせず[...]ボタンを押下すると発生しないらしい。OnUpdateValue() も1回だけコールバックされる。
		// CMFCPropertyGridFileProperty や CMFCPropertyGridFontProperty も同様の現象が発生する。
		// AFX_WM_PROPERTY_CHANGED の受信側でよろしく対処するしかないかも。
	}

	// [Cancel] が選択された場合は何もしない。
	// いったんテキスト化してシリアライズしてしまったほうが、コピー＆ペーストで他のマテリアルに対して同様の情報を一発設定しやすくなる。
	// レンダリング時の高速化のために、GDI+ 用グラデーション ストップの配列（バイナリ）も参照できるようにあらかじめ解析しておいたほうがよい。
	// トゥーン グラデーション テクスチャを作成する場合も、プレーンテキストでなく解析済みの GDI+ 用バイナリ配列データから取得したほうがよい。
}


void CMyGradientStopsProperty::OnDrawValue(CDC* pDC, CRect rect)
{
	// GDI+ の LinearGradientBrush もしくは GDI の GradientFill() を使ってグラデーションを描画する。
	// ネイティブ Win32 コード主導の場合、ここで Direct2D や WPF を使うのはさすがにオーバースペック。
	// なお、2色グラデーションまでであれば、Feature Pack で追加された CDrawingManager を使うと楽だが、ここでは役不足。

	//__super::OnDrawValue(pDC, rect);
	//pDC->FillSolidRect(rect, RGB(0, 0xFF, 0xFF));

	const COLORREF oldTextColor = pDC->GetTextColor();
	if (lstrlenW(m_varValue.bstrVal) != 0)
	{
		if (m_gradStopOffsets.empty())
		{
			pDC->SetTextColor(RGB(0xFF, 0, 0)); // 不正フォーマットの場合は赤字。
		}
		else
		{
			std::shared_ptr<Gdiplus::Graphics> grfx(Gdiplus::Graphics::FromHDC(pDC->GetSafeHdc()));

			const Gdiplus::Rect gradRect1(rect.left, 0, rect.Width() - 1, 1);
			const Gdiplus::Rect gradRect2(gradRect1.X + 1, rect.top + 1, gradRect1.Width - 1, rect.Height() - 1);
			// 2色グラデーションまではコンストラクタだけで指定可能。3色以上は InterpolationColors を使う。
			// とりあえず適当な2色をセットしておく。
			Gdiplus::LinearGradientBrush gradBrush(
				gradRect1,
				Gdiplus::Color::Crimson, Gdiplus::Color::DodgerBlue, 0);
			gradBrush.SetWrapMode(Gdiplus::WrapModeClamp);
			// ブラシの開始点と終了点を表す矩形は正規化されていなくて、絶対座標らしい。
			// WPF の System.Windows.Media.LinearGradientBrush.StartPoint と EndPoint は正規化されているが、
			// Direct2D の ID2D1LinearGradientBrush の StartPoint と EndPoint は GDI+ と同じらしい。
			// 整数ピクセル単位だと、WrapMode を Clamp にしていても条件によっては左右の端に互いの色が出現することがある。
			// 矩形を描画するときにブラシ矩形から左右1ピクセルずつ減らしてクリッピングしたほうがよさげ。
			gradBrush.SetGammaCorrection(true);
			// GDI+ の LinearGradientBrush の InterpolationColors を使う場合、
			// 最初と最後の Stop オフセット位置は必ずそれぞれ 0, 1 でなければならないらしい。
			// WPF/Photoshop の場合、最初と最後の Stop オフセットはそれぞれ 0, 1 でなくても良い（左端は最初の色、右端は最後の色になる）ので、
			// GDI+ で描画する場合は注意が必要。
#if 0
			// テストコード。
			const int gradStopCount = 2;
			const Gdiplus::Color gradStopColors[gradStopCount] =
			{
				Gdiplus::Color::Gold,
				Gdiplus::Color::Lime,
			};
			const float gradStopOffsets[gradStopCount] =
			{
				0.00f,
				//0.25f,
				//0.50f,
				//0.75f,
				1.00f,
			};
			gradBrush.SetInterpolationColors(gradStopColors, gradStopOffsets, gradStopCount);
			grfx->FillRectangle(&gradBrush, gradRect2);
#else
			_ASSERTE(m_gradStopOffsets.size() == m_gradStopColors.size());
			const int gradStopCount = int(m_gradStopOffsets.size());
			gradBrush.SetInterpolationColors(&m_gradStopColors[0], &m_gradStopOffsets[0], gradStopCount);
			grfx->FillRectangle(&gradBrush, gradRect2);
#endif
		}
	}
	// MFC 11.0 の既定の CMFCPropertyGridProperty::OnDrawValue() 実装では、DrawText() で文字列が描画されている。
	__super::OnDrawValue(pDC, rect);
	pDC->SetTextColor(oldTextColor); // 元の色に戻す。
}

#pragma endregion
