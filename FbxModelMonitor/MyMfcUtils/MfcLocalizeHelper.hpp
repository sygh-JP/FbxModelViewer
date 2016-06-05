#pragma once


namespace Misc
{
	//! @brief  ローカライズに関するヘルパーを提供する名前空間。<br>
	namespace LocalizeHelper
	{
		//! @brief  MFC の基本言語に対応する、言語リソース型。<br>
		enum LanguageResType
		{
			LanguageResType_Neutral,  //!< ニュートラル（英語、米国）。<br>
			LanguageResType_Jpn, //!< 日本語（日本）。<br>
			LanguageResType_Fra, //!< フランス語（フランス）。<br>
			LanguageResType_Deu, //!< ドイツ語（ドイツ）。<br>
			LanguageResType_Chs, //!< 簡体字中国語。<br>
			LanguageResType_Cht, //!< 繁体字中国語。<br>
			LanguageResType_All
		};


		const DWORD LangID_en_US   = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
		const DWORD LangID_ja_JP   = MAKELANGID(LANG_JAPANESE, SUBLANG_JAPANESE_JAPAN);
		const DWORD LangID_fr_FR   = MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH);
		const DWORD LangID_de_DE   = MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN);
		const DWORD LangID_zh_Hans = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
		const DWORD LangID_zh_Hant = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL);


		//! @brief  FormatMessage() API などに渡す言語 ID を取得する。<br>
		inline DWORD GetLangIDFromLanguageResType(LanguageResType resType)
		{
			switch (resType)
			{
			case LanguageResType_Jpn:
				return LangID_ja_JP;
			case LanguageResType_Fra:
				return LangID_fr_FR;
			case LanguageResType_Deu:
				return LangID_de_DE;
			case LanguageResType_Chs:
				return LangID_zh_Hans;
			case LanguageResType_Cht:
				return LangID_zh_Hant;
			default: // ニュートラルその他は英語扱い。
				return LangID_en_US;
			}
		}

		inline WORD GetCurrentUserDefaultPrimaryLangID()
		{
			// ちなみに GetACP() で「OS の現在の ANSI コード ページ」を取得できるが、言語までは判断できない。
			return PRIMARYLANGID(::GetUserDefaultLangID()); // こちらは日本語版 Windows にて MUI で英語 UI に変更したとき、LANG_JAPANESE が返る。
			//return PRIMARYLANGID(::GetUserDefaultLCID()); // こちらは日本語版 Windows にて MUI で英語 UI に変更したとき、LANG_JAPANESE が返る。
		}

		inline WORD GetCurrentUserDefaultPrimaryUILanguage()
		{
			return PRIMARYLANGID(::GetUserDefaultUILanguage()); // こちらは MUI 適用環境でも UI 言語を判定できるはず。
		}

		inline LanguageResType GetLanguageResTypeByCurrentUserDefaultPrimaryLangID()
		{
			const WORD langID = GetCurrentUserDefaultPrimaryLangID();
			switch (langID)
			{
			case LANG_JAPANESE:
				return LanguageResType_Jpn;
			case LANG_FRENCH:
				return LanguageResType_Fra;
			case LANG_GERMAN:
				return LanguageResType_Deu;
			case LANG_CHINESE_SIMPLIFIED:
				return LanguageResType_Chs;
			case LANG_CHINESE_TRADITIONAL:
				return LanguageResType_Cht;
				// TODO: 他言語の対応。
			default:
				return LanguageResType_Neutral;
			}
		}

		inline LanguageResType GetLanguageResTypeByCurrentUserDefaultPrimaryUILanguage()
		{
			const WORD langID = GetCurrentUserDefaultPrimaryUILanguage();
			switch (langID)
			{
			case LANG_JAPANESE:
				return LanguageResType_Jpn;
			case LANG_FRENCH:
				return LanguageResType_Fra;
			case LANG_GERMAN:
				return LanguageResType_Deu;
			case LANG_CHINESE_SIMPLIFIED:
				return LanguageResType_Chs;
			case LANG_CHINESE_TRADITIONAL:
				return LanguageResType_Cht;
				// TODO: 他言語の対応。
			default:
				return LanguageResType_Neutral;
			}
		}


		//! @brief  MFC のローカライズ用サテライト DLL 関連の名前空間。<br>
		namespace MfcSatelliteDll
		{
			const LPCWSTR PostfixStrNeutral = L"";
			const LPCWSTR PostfixStrJpn = L"JPN";
			const LPCWSTR PostfixStrFra = L"FRA";
			const LPCWSTR PostfixStrDeu = L"DEU";
			const LPCWSTR PostfixStrChs = L"CHS";
			const LPCWSTR PostfixStrCht = L"CHT";

			//! @brief  NxLanguageResType に対応した言語 DLL ポストフィックス文字列のテーブル。<br>
			const LPCWSTR PostfixStringsTable[LanguageResType_All] =
			{
				PostfixStrNeutral,
				PostfixStrJpn,
				PostfixStrFra,
				PostfixStrDeu,
				PostfixStrChs,
				PostfixStrCht,
			};

			//! @brief  インデックスを指定して、文字列テーブルから対応する文字列を取得するヘルパー関数。<br>
			inline LPCWSTR GetPostfixStringInTable(LanguageResType langResType)
			{
				ATLASSERT(0 <= langResType && langResType < LanguageResType_All);
				return PostfixStringsTable[langResType];
			}

			inline LanguageResType GetLangResTypeByPostfixString(CStringW strPostfix)
			{
				strPostfix.MakeUpper();
				for (int i = 0; i < LanguageResType_All; ++i)
				{
					if (strPostfix == PostfixStringsTable[i])
					{
						return static_cast<LanguageResType>(i);
					}
				}
				return LanguageResType_Neutral;
			}

		} // end of namespace


		//! @brief  ローカライズされたカルチャー関連の名前空間。主に .NET 用。<br>
		namespace LocalizedCulture
		{
			const LPCWSTR CultureNameStr_en_US    = L"en-US";
			const LPCWSTR CultureNameStr_ja_JP    = L"ja-JP";
			const LPCWSTR CultureNameStr_fr_FR    = L"fr-FR";
			const LPCWSTR CultureNameStr_de_DE    = L"de-DE";
			const LPCWSTR CultureNameStr_zh_Hans  = L"zh-Hans";
			const LPCWSTR CultureNameStr_zh_Hant  = L"zh-Hant";

			const LPCWSTR CultureNameStrNeutral = CultureNameStr_en_US;

			//! @brief  NxLanguageResType に対応したカルチャー名文字列のテーブル。<br>
			const LPCWSTR CultureNameStringsTable[LanguageResType_All] =
			{
				CultureNameStrNeutral,
				CultureNameStr_ja_JP,
				CultureNameStr_fr_FR,
				CultureNameStr_de_DE,
				CultureNameStr_zh_Hans,
				CultureNameStr_zh_Hant,
			};

			//! @brief  インデックスを指定して、文字列テーブルから対応する文字列を取得するヘルパー関数。<br>
			inline LPCWSTR GetCultureNameStringInTable(LanguageResType langResType)
			{
				ATLASSERT(0 <= langResType && langResType < LanguageResType_All);
				return CultureNameStringsTable[langResType];
			}

			inline LanguageResType GetLangResTypeByCultureName(CStringW strCultureName)
			{
				for (int i = 0; i < LanguageResType_All; ++i)
				{
					if (strCultureName.CompareNoCase(CultureNameStringsTable[i]) == 0)
					{
						return static_cast<LanguageResType>(i);
					}
				}
				return LanguageResType_Neutral;
			}

		} // end of namespace


		inline LPCWSTR MapCultureNameToMfcPostfixString(CStringW strCultureName)
		{
			return MfcSatelliteDll::GetPostfixStringInTable(LocalizedCulture::GetLangResTypeByCultureName(strCultureName));
		}

		inline LPCWSTR MapMfcPostfixStringToCultureName(CStringW strPostfix)
		{
			return LocalizedCulture::GetCultureNameStringInTable(MfcSatelliteDll::GetLangResTypeByPostfixString(strPostfix));
		}


		//! @brief  ANSI コード ページ番号。<br>
		enum AnsiCodePage
		{
			AnsiCodePage_Japanese = 932,
			AnsiCodePage_WindowsLatin1 = 1252,
			AnsiCodePage_ChineseHanSimple = 936,
			AnsiCodePage_ChineseHanTraditional = 950,
		};


		//! @brief  言語リソース インスタンスのパッケージ クラス。<br>
		class LanguageResInstancePack sealed
		{
		private:
			HINSTANCE m_hResInstances[LanguageResType_All];
			LanguageResType m_currentLangResType;
			bool m_usesWinLogInUserLang;
		public:
			LanguageResInstancePack()
				: m_hResInstances()
				, m_currentLangResType()
				, m_usesWinLogInUserLang()
			{}

			~LanguageResInstancePack()
			{}

		public:
			HINSTANCE GetLanguageResInstance(LanguageResType langResType) const
			{
				ATLASSERT(0 <= langResType && langResType < LanguageResType_All);
				return m_hResInstances[langResType];
			}

			bool GetIsLanguageResInstanceLoaded(LanguageResType langResType) const
			{ return this->GetLanguageResInstance(langResType) != NULL; }

			LanguageResType GetCurrentLanguageResType() const
			{ return m_currentLangResType; }

			HINSTANCE GetCurrentLanguageResInstance() const
			{ return this->GetLanguageResInstance(this->GetCurrentLanguageResType()); }

			//! @brief  現在 MFC に設定されている言語リソース タイプから、対応するカルチャ名を取得する。<br>
			LPCWSTR GetCurrentCultureName() const
			{ return LocalizedCulture::GetCultureNameStringInTable(m_currentLangResType); }

			//! @brief  現在 MFC に設定されている言語リソース タイプから、対応する言語 DLL ポストフィックスを取得する。<br>
			LPCWSTR GetCurrentPostfix() const
			{ return MfcSatelliteDll::GetPostfixStringInTable(m_currentLangResType); }

			//! @brief  現在 MFC に設定されている言語リソース タイプから、対応する言語 ID を取得する。<br>
			DWORD GetCurrentLangID() const
			{ return GetLangIDFromLanguageResType(m_currentLangResType); }

			bool GetUsesWinLogInUserLanguage() const
			{ return m_usesWinLogInUserLang; }


			//! @brief  現在のリソースをニュートラルのリソースとして登録する。<br>
			//! @attention  EXE 側にニュートラル リソースがあるならば、<br>
			//! EXE 起動直後、サテライト DLL を自動読込していない状態で呼び出す必要がある。<br>
			void UpdateNeutralResInstanceByCurrentResInstance()
			{
				this->m_hResInstances[LanguageResType_Neutral] = AfxGetResourceHandle();
			}

			bool ChangeCurrentResInstance(LanguageResType newLangResType)
			{
				HINSTANCE hNewInst = this->GetLanguageResInstance(newLangResType);
				if (hNewInst)
				{
					m_currentLangResType = newLangResType;
					m_usesWinLogInUserLang = false; // いったん問答無用で false 設定にしておく。
					AfxSetResourceHandle(hNewInst);
					return true;
				}
				return false;
			}

			//! @brief  現在の Windows ログイン ユーザーのデフォルト UI 言語 ID から、<br>
			//! 対応するリソース言語を判定し、（対応するリソース DLL が存在して読み込まれていれば）MFC に設定する。<br>
			bool ChangeCurrentResInstanceByCurrentUserDefaultPrimaryLangID()
			{
				m_usesWinLogInUserLang = this->ChangeCurrentResInstance(GetLanguageResTypeByCurrentUserDefaultPrimaryUILanguage());
				return m_usesWinLogInUserLang;
			}

			//! @brief  存在するすべての言語リソース DLL をロードする。<br>
			void LoadAllLanguageResDlls(LPCTSTR pBaseAppName)
			{
				//ATLASSERT(!m_hResInstances[NxLanguageResType_Jpn]);

				// EXE と同じ階層に言語リソース {AppName}???.dll を配置しておくと、MFC のサテライト DLL 機構によって
				// 自動的に OS 言語環境に応じた DLL がロードされるようになるが、
				// ユーザーの明示的指定によってリソースを切り替える機能を実現する必要がある場合、
				// サブ ディレクトリに配置した言語リソースに対して明示的に
				// AfxLoadLibrary() + AfxSetResourceHandle() を使用する。
				CString strDllPathName;
				// サブ ディレクトリ名は .NET のカルチャ名に準じる。
				for (int i = LanguageResType_Jpn; i < LanguageResType_All; ++i)
				{
					//if (!m_hResInstances[i])
					{
						strDllPathName.Format(_T("%s\\%s%s.dll"),
							LocalizedCulture::CultureNameStringsTable[i], pBaseAppName, MfcSatelliteDll::PostfixStringsTable[i]);
						m_hResInstances[i] = AfxLoadLibrary(strDllPathName); // 参照カウントが増加される。
					}
				}
			}

			//! @brief  ロードしたすべての言語リソースを解放する。<br>
			//! 
			//! @deprecated  ただしリソース DLL を使用中に解放しようとするのはまずいので、<br>
			//! ロードに関しては明示的に1回だけ行ない、解放に関してはできるかぎり明示的に呼び出さず、<br>
			//! Windows OS のローダーにまかせたほうがよい。<br>
			__declspec(deprecated) void FreeAllLangurageResDlls()
			{
				for (int i = 1; i < LanguageResType_All; ++i)
				{
					if (m_hResInstances[i])
					{
						AfxFreeLibrary(m_hResInstances[i]); // 参照カウントが減少される。
						m_hResInstances[i] = NULL;
					}
				}
			}
		};

	} // end of namespace
} // end of namespace
