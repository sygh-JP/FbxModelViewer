#include "stdafx.h"
#include "MyXmlProc.hpp"

using MyMisc::CustomComException;
using namespace MyMisc::Xml;

namespace
{
	void EnableSchemaCheck(_Inout_ MSXML2::IXMLDOMDocument2Ptr pDoc, _In_ const _bstr_t& strNamespaceUri, _In_ const _bstr_t& strSchemaPath)
	{
		MSXML2::IXMLDOMSchemaCollection2Ptr pSchema;
		HRESULT hr = CreateXmlDomSchema(pSchema);
		if (FAILED(hr))
		{
			ATLTRACE(__FILEW__ L"(%d): Failed to create XML schema instance!!\n", __LINE__);
			throw CustomComException(hr, L"Failed to create XML schema instance!!");
		}

		try
		{
			pSchema->add(strNamespaceUri, strSchemaPath);
			pDoc->schemas = pSchema.GetInterfacePtr();
			pDoc->validateOnParse = VARIANT_TRUE;
			pDoc->resolveExternals = VARIANT_TRUE;
		}
		catch (const _com_error& e)
		{
			ATLTRACE(__FILEW__ L"(%d): XML schema COM error!! (0x%08X = %s)\n", __LINE__, e.Error(), CStringW(e.ErrorMessage()));
			throw CustomComException(e.Error(), e.ErrorMessage());
		}
		catch (...)
		{
			ATLTRACE(__FILEW__ L"(%d): XML schema unknown error!!\n", __LINE__);
			throw CustomComException(E_FAIL, L"XML schema unknown error!!");
		}
	}
} // end of namespace


namespace MyMisc
{
	namespace Xml
	{
		HRESULT CreateXmlDomDocument(MSXML2::IXMLDOMDocument2Ptr& pDoc)
		{
			const HRESULT hr = pDoc.CreateInstance(MSXML2::CLSID_DOMDocument60);
			return hr;
		}


		HRESULT CreateXmlDomSchema(MSXML2::IXMLDOMSchemaCollection2Ptr& pSchema)
		{
			const HRESULT hr = pSchema.CreateInstance(MSXML2::CLSID_XMLSchemaCache60);
			return hr;
		}

		HRESULT CreateMxXmlWriter(MSXML2::IMXWriterPtr& pWriter)
		{
			const HRESULT hr = pWriter.CreateInstance(MSXML2::CLSID_MXXMLWriter60);
			return hr;
		}

		HRESULT CreateSaxXmlReader(MSXML2::ISAXXMLReaderPtr& pReader)
		{
			const HRESULT hr = pReader.CreateInstance(MSXML2::CLSID_SAXXMLReader60);
			return hr;
		}


		void CreateElementOfDocRoot(_In_ const _bstr_t& strXml, bool usesStrXmlAsXmlFilePath, _Out_ MSXML2::IXMLDOMElementPtr& pElemRoot, _In_ bool enablesXPath, _In_ const _bstr_t& strNamespaceUri, _In_ const _bstr_t& strSchemaPath)
		{
			MSXML2::IXMLDOMDocument2Ptr pDoc;
			HRESULT hr = CreateXmlDomDocument(pDoc);
			if (FAILED(hr))
			{
				ATLTRACE(__FILEW__ L"(%d): Failed to create XML document instance!!\n", __LINE__);
				throw CustomComException(hr, L"Failed to create XML document instance!!");
			}
			pDoc->async = VARIANT_FALSE; // load メソッドが完了するまで待つ。

			if (strSchemaPath.length() != 0)
			{
				// スキーマ チェックを行なう。
				try
				{
					EnableSchemaCheck(pDoc, strNamespaceUri, strSchemaPath);
				}
				catch (...)
				{
					throw;
				}
			}

			bool isLoadSucceeded = false;
			if (usesStrXmlAsXmlFilePath)
			{
				_ASSERTE(::PathFileExistsW(strXml));
				isLoadSucceeded = (VARIANT_FALSE != pDoc->load(strXml));
			}
			else
			{
				isLoadSucceeded = (VARIANT_FALSE != pDoc->loadXML(strXml));
			}
			if (!isLoadSucceeded)
			{
				MSXML2::IXMLDOMParseErrorPtr pPsErr = pDoc->parseError;

				CStringW strErrMsg;
				strErrMsg.Format(L"Failed to load XML document!! Reason: [\r\n%s], Source: [%s], Line: (%ld)",
					static_cast<LPCWSTR>(pPsErr->Getreason()), static_cast<LPCWSTR>(pPsErr->GetsrcText()), pPsErr->Getline());
				ATLTRACE(__FILEW__ L"(%d): %s\n", __LINE__, strErrMsg.GetString());
				throw CustomComException(E_FAIL, strErrMsg.GetString());
			}

			pElemRoot = pDoc->documentElement;
			if (!pElemRoot)
			{
				ATLTRACE(__FILEW__ L"(%d): Failed to get XML document element root!!\n", __LINE__);
				throw CustomComException(E_FAIL, L"Failed to get XML document element root!!");
			}

			if (enablesXPath)
			{
				// XPath を使うように設定。
				hr = pDoc->setProperty(L"SelectionLanguage", L"XPath");
				if (hr != S_OK)
				{
					ATLTRACE(__FILEW__ L"(%d): Failed to enable XPath!!\n", __LINE__);
					throw CustomComException(E_FAIL, L"Failed to enable XPath!!");
				}
			}
		}


		void SaveIndentedXmlToFile(_In_ const _bstr_t& strXmlFullPath, _In_ MSXML2::IXMLDOMDocument2Ptr pDoc, _In_ const _bstr_t& strNamespaceUri, _In_ const _bstr_t& strSchemaPath)
		{
			if (strSchemaPath.length() != 0)
			{
				// スキーマ チェックを行なう。
				try
				{
					EnableSchemaCheck(pDoc, strNamespaceUri, strSchemaPath);
				}
				catch (...)
				{
					throw;
				}
			}

			// UNDONE: 現状では保存時のスキーマ チェックは未実装。

			try
			{
				// 書式化出力インターフェイスにてインデント処理して XML 文書を保存する。
				IStreamPtr smFile;

				HRESULT hr = ::SHCreateStreamOnFileW(
					strXmlFullPath,
					STGM_READWRITE | STGM_SHARE_DENY_WRITE | STGM_CREATE,
					&smFile);

				// ファイルの作成に失敗した場合。
				if (FAILED(hr))
				{
					ATLTRACE(__FILEW__ L"(%d): Failed to create XML file stream!!\n", __LINE__);
					throw CustomComException(hr, L"Failed to create XML file stream!!");
				}

				// ライターの作成。
				MSXML2::IMXWriterPtr pWriter;
				hr = CreateMxXmlWriter(pWriter);
				if (FAILED(hr))
				{
					ATLTRACE(__FILEW__ L"(%d): Failed to create XML writer!!\n", __LINE__);
					throw CustomComException(hr, L"Failed to create XML writer!!");
				}

				// ヘッダーをセットする。
				pWriter->version = L"1.0";
				pWriter->encoding = L"UTF-8";
				pWriter->standalone = VARIANT_TRUE; // true ==> "yes", false ==> "no"
				// インデントを有効にする。
				pWriter->indent = VARIANT_TRUE;
				pWriter->output = _variant_t(static_cast<IUnknown*>(smFile.GetInterfacePtr()));

				// リーダーの作成。
				MSXML2::ISAXXMLReaderPtr pReader;
				hr = CreateSaxXmlReader(pReader);
				if (FAILED(hr))
				{
					ATLTRACE(__FILEW__ L"(%d): Failed to create XML reader!!\n", __LINE__);
					throw CustomComException(hr, L"Failed to create XML reader!!");
				}

				pReader->putContentHandler(static_cast<MSXML2::ISAXContentHandlerPtr>(pWriter));
				// パーシングと実際の書込。
				pReader->parse(_variant_t(static_cast<IUnknown*>(pDoc.GetInterfacePtr())));
			}
			catch (const _com_error& e)
			{
				ATLTRACE(__FILEW__ L"(%d): XML output COM error!! (0x%08X = %s)\n", __LINE__, e.Error(), CStringW(e.ErrorMessage()));
				throw CustomComException(e.Error(), e.ErrorMessage());
			}
			catch (...)
			{
				throw;
			}
		}


		void SaveBodyToFile(_In_ const _bstr_t& strXmlFullPath, _In_ const _bstr_t& strXmlBody, _In_ const _bstr_t& strNamespaceUri, _In_ const _bstr_t& strSchemaPath)
		{
			// ドキュメントのインスタンスを作成する。
			MSXML2::IXMLDOMDocument2Ptr pDoc;
			HRESULT hr = CreateXmlDomDocument(pDoc);
			if (FAILED(hr))
			{
				ATLTRACE(__FILEW__ L"(%d): Failed to create XML document instance!!\n", __LINE__);
				throw CustomComException(hr, L"Failed to create XML document instance!!");
			}
			// ホワイトスペースのみのテキストノードがノードのインデントとして整形されて読み込まれるようにする。
			// （デフォルトではインデントされない）
			pDoc->preserveWhiteSpace = VARIANT_FALSE;

			// ドキュメントに文字列を供給する。
			const VARIANT_BOOL result = pDoc->loadXML(strXmlBody);
			try
			{
				SaveIndentedXmlToFile(strXmlFullPath, pDoc);
			}
			catch (...)
			{
				throw;
			}
		}


		void GetXmlItemByXPath(MSXML2::IXMLDOMElementPtr pElemRoot, LPCWSTR pStringXPath, CStringW& strOut)
		{
			//ATLTRACE(__FILEW__ L"(%d): " __FUNCTIONW__ L"()\n", __LINE__);
			ATLTRACE(L"XPath = \"%s\"\n", pStringXPath);

			_ASSERTE(pElemRoot);

			MSXML2::IXMLDOMNodePtr pNode = pElemRoot->selectSingleNode(pStringXPath);
			if (!pNode)
			{
				ATLTRACE(__FILEW__ L"(%d): XPath is invalid!!\n", __LINE__);
				throw CustomComException(E_INVALIDARG, L"XPath is invalid!!");
			}

			MSXML2::IXMLDOMNodePtr pTarget;
			// XPath を使用するようにドキュメントのプロパティを設定していなかった場合など、COM の例外が送出される。
			try
			{
				pTarget = pNode->firstChild;
			}
			catch (const _com_error& e)
			{
				ATLTRACE(L"XPath COM error!! (0x%08lX = %s)\n", e.Error(), CStringW(e.ErrorMessage()));
				throw CustomComException(e.Error(), e.ErrorMessage());
			}
			catch (...)
			{
				throw CustomComException(E_FAIL, L"XPath unknown error!!");
			}

			_bstr_t varText;
			try
			{
				// XPath で取得したい属性値が空の場合、また "/>" で終わる空要素の値に対する XPath であった場合も、
				// pTarget スマートポインタが NULL となり、逆参照すると内部で COM 例外が発生する。
				// NULL チェックしておけば問題ないはずだが、念のため例外処理も付ける。
				if (pTarget)
				{
					varText = pTarget->text;
				}
			}
			catch (const _com_error& e)
			{
				ATLTRACE(L"XPath COM error!! (0x%08lX = %s)\n", e.Error(), CStringW(e.ErrorMessage()));
				throw CustomComException(e.Error(), e.ErrorMessage());
			}
			catch (...)
			{
				throw CustomComException(E_FAIL, L"XPath unknown error!!");
			}

			strOut = static_cast<LPCWSTR>(varText);
		}


		ThreeValueBoolean GetBooleanValueFromString(const CStringW& strTarget)
		{
			const bool isTrue = strTarget.CompareNoCase(L"true") == 0 || strTarget.Compare(L"1") == 0;
			if (isTrue)
			{
				return ThreeValueBoolean_True;
			}
			const bool isFalse = strTarget.CompareNoCase(L"false") == 0 || strTarget.Compare(L"0") == 0;
			if (isFalse)
			{
				return ThreeValueBoolean_False;
			}
			return ThreeValueBoolean_Invalid;
		}

	} // end of namespace
} // end of namespace
