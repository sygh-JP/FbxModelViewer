#pragma once


namespace MyMisc
{
	//! @brief  COM エラーを報告するカスタム例外クラス。<br>
	class CustomComException
	{
	private:
		HRESULT m_hrErrCode; //!< COM HRESULT エラーコード。<br>
		CStringW m_strErrMsg; //!< エラーメッセージ。<br>

	public:
		HRESULT GetErrorCode() const { return m_hrErrCode; }
		const CStringW& GetErrorMessage() const { return m_strErrMsg; }

	private:
		CustomComException();

	public:
		explicit CustomComException(HRESULT hrErrCode)
			: m_hrErrCode(hrErrCode)
		{}

		CustomComException(HRESULT hrErrCode, LPCWSTR pErrMsg)
			: m_hrErrCode(hrErrCode)
			, m_strErrMsg(pErrMsg)
		{}

		CustomComException(HRESULT hrErrCode, LPCSTR pErrMsg)
			: m_hrErrCode(hrErrCode)
			, m_strErrMsg(pErrMsg)
		{}
	};


	//! @brief  XML ユーティリティ名前空間。<br>
	//! 
	//! XML の解析には MSXML 6.0 を使用するので、システムへのインストールおよび COM の初期化を事前に行なっておくこと。<br>
	//! ちなみに 64bit 版の MSXML 4.0 は存在しない。<br>
	//! @attention  Create～ファクトリ関数をわざわざ用意したのは、CLSID をあちこち変更して回るのが面倒なため。<br>
	namespace Xml
	{
		//! @brief  XML DOM ドキュメント インスタンスを作成してインターフェイス ポインタを返す。<br>
		extern HRESULT CreateXmlDomDocument(MSXML2::IXMLDOMDocument2Ptr& pDoc);

		//! @brief  XML DOM スキーマ インスタンスを作成してインターフェイス ポインタを返す。<br>
		extern HRESULT CreateXmlDomSchema(MSXML2::IXMLDOMSchemaCollection2Ptr& pSchema);

		extern HRESULT CreateMxXmlWriter(MSXML2::IMXWriterPtr& pWriter);

		extern HRESULT CreateSaxXmlReader(MSXML2::ISAXXMLReaderPtr& pReader);


		const LPCWSTR AttrNameStrXmlNamespaceXmlSchemaInstance = L"xmlns:xsi";
		const LPCWSTR AttrValueStrXmlNamespaceXmlSchemaInstance = L"http://www.w3.org/2001/XMLSchema-instance";
		const LPCWSTR AttrNameStrXsiNoNamespaceSchemaLocation = L"xsi:noNamespaceSchemaLocation";


		//! @brief  XML 文字列あるいは XML ファイルから XML DOM ドキュメント ルートを作成して返す。<br>
		//! 
		//! @param[in]  strXml  入力する XML 文字列あるいは XML ファイルのフルパスを指定する。<br>
		//! @param[in]  usesStrXmlAsXmlFilePath  第1引数を、入力する XML ファイルのフルパスとして扱う。<br>
		//! @exception  CustomComException  エラーが発生した場合にスローされる。<br>
		extern void CreateElementOfDocRoot(_In_ const _bstr_t& strXml, bool usesStrXmlAsXmlFilePath, _Out_ MSXML2::IXMLDOMElementPtr& pElemRoot, _In_ bool enablesXPath = false, _In_ const _bstr_t& strNamespaceUri = _bstr_t(), _In_ const _bstr_t& strSchemaPath = _bstr_t());


		//! @brief  XML DOM ドキュメントをインデント（自動フォーマット）された XML ファイルとして保存する。<br>
		//! 
		//! 保存時のスキーマチェックは未実装。<br>
		//! @param[in]  strXmlFullPath  出力される XML ファイルのフルパスを指定する。<br>
		//! @exception  CustomComException  エラーが発生した場合にスローされる。<br>
		extern void SaveIndentedXmlToFile(_In_ const _bstr_t& strXmlFullPath, _In_ MSXML2::IXMLDOMDocument2Ptr pDoc, _In_ const _bstr_t& strNamespaceUri = _bstr_t(), _In_ const _bstr_t& strSchemaPath = _bstr_t());

		//! @brief  XML ボディ文字列をインデント（自動フォーマット）された XML ファイルとして保存する。<br>
		//! 
		//! 保存時のスキーマチェックは未実装。<br>
		//! @param[in]  strXmlFullPath  出力される XML ファイルのフルパスを指定する。<br>
		//! @param[in]  strXmlBody  ヘッダを含まない XML ボディのみ指定する。<br>
		//! @exception  CustomComException  エラーが発生した場合にスローされる。<br>
		extern void SaveBodyToFile(_In_ const _bstr_t& strXmlFullPath, _In_ const _bstr_t& strXmlBody, _In_ const _bstr_t& strNamespaceUri = _bstr_t(), _In_ const _bstr_t& strSchemaPath = _bstr_t());


		extern void GetXmlItemByXPath(MSXML2::IXMLDOMElementPtr pElemRoot, LPCWSTR pStringXPath, CStringW& strOut);


#pragma region // XML Boolean //

		//! @brief  文字列を bool 値に変換するときに使われる中間型。いわゆる 3 値論理型。<br>
		enum ThreeValueBoolean
		{
			ThreeValueBoolean_Invalid = -1,
			ThreeValueBoolean_False   =  0,
			ThreeValueBoolean_True    = +1,
		};

		inline LPCWSTR ToAlphaStringW(bool target)
		{ return target ? L"true" : L"false"; }

		inline LPCWSTR ToIntStringW(bool target)
		{ return target ? L"1" : L"0"; }

		//! @brief  文字列を 3 値論理型に変換する。大文字・小文字を区別しない。<br>
		//! 
		//! @retval  ThreeValueBoolean_True     入力文字列が "true" あるいは "1" の場合。<br>
		//! @retval  ThreeValueBoolean_False    入力文字列が "false" あるいは "0" の場合。<br>
		//! @retval  ThreeValueBoolean_Invalid  入力文字列が上記以外の場合。<br>
		extern ThreeValueBoolean GetBooleanValueFromString(const CStringW& strTarget);

#pragma endregion
	}
}
