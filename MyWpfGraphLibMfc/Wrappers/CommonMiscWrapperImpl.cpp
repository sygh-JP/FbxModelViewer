#include "stdafx.h"
#include "../stdafx.h" // インテリセンス用の冗長インクルード。
//#include "../CppCliInterop.hpp"
#include "../CppCliMisc.hpp"
#include "CommonMiscWrapper.hpp"

#include "../../SharedUtility/PublicInclude/DebugNew.h"


namespace MyWpfGraphLibWrapper
{
	namespace CommonMisc
	{
		void ExecuteGarbageCollect()
		{
			System::GC::Collect();
		}

		void SetWpfResourcesCulture(CStringW strCultureName)
		{
			MyWpfGraphLibrary::CultureResources::SetResoucesCulture(CppCliMisc::CreateCultureInfoByName(strCultureName));
		}

		void DoWpfEvents()
		{
			MyMiscHelpers::MyThreadHelper::DoEvents();
		}

		bool GetCurrentKeyboardFocusedWpfControlExists()
		{
			return MyMiscHelpers::MyInputHelper::GetCurrentKeyboardFocusedControlExists();
		}

	} // end of namespace
} // end of namespace
