#include "stdafx.h"

#include "FbxAnimTimeInfo.h"
#include "MyUtil.h"


namespace MyFbx
{
	MyFbxAnimTimeInfo::MyFbxAnimTimeInfo()
		: m_framesPerSec()
	{
	}

	MyFbxAnimTimeInfo::~MyFbxAnimTimeInfo()
	{
	}

	void MyFbxAnimTimeInfo::Analyze(const FbxScene* scene)
	{
		// FbxTakeInfo は廃止されずに残っているらしいが、
		// 代替として新設された FbxAnimStack や FbxAnimLayer を使って同等の情報を取得することができるらしい。
		// また、トラックごとのタイム インターバルなどは、FbxAnimStack と FbxAnimEvaluator を使わないと取得できない模様。
		// たとえスキニング メッシュであっても、アニメーションのないノードもあるので、時間情報の取得はノードごとに行なうしかない？
		const int animStackCount = scene->GetSrcObjectCount<FbxAnimStack>();
		ATLTRACE("AnimStackCount = %d\n", animStackCount);
		for (int a = 0; a < animStackCount; ++a)
		{
			const auto* pAnimStack = scene->GetSrcObject<const FbxAnimStack>(a);
			const char* pAnimStackName = pAnimStack->GetName();
			const auto strAnimTrackName = MyUtils::SafeConvertUtf8toUtf16(pAnimStackName);
			ATLTRACE(L"AnimTrackName[%d] = \"%s\"\n", a, strAnimTrackName.c_str());
			const auto localTimeStart = pAnimStack->GetLocalTimeSpan().GetStart();
			const auto localTimeStop = pAnimStack->GetLocalTimeSpan().GetStop();
			const auto refTimeStart = pAnimStack->GetReferenceTimeSpan().GetStart();
			const auto refTimeStop = pAnimStack->GetReferenceTimeSpan().GetStop();
			ATLTRACE("LocalTimeStart = %I64d, LocalTimeStop = %I64d, RefTimeStart = %I64d, RefTimeStop = %I64d\n",
				localTimeStart.Get(), localTimeStop.Get(), refTimeStart.Get(), refTimeStop.Get());
			m_animStackNamesArray.push_back(strAnimTrackName);
		}

		// 時間モードから単位時間を算出。
		//KFbxGlobalTimeSettings& globalTimeSettings = scene->GetGlobalTimeSettings(); // FBX SDK 2011.3.1 の時点ではまだ削除されていないが、廃止予定らしいので代替の方法に切り替えておく。
		const auto& globalSettings = scene->GetGlobalSettings();
		const auto timeMode = globalSettings.GetTimeMode();
		m_animPeriod.SetTime(0, 0, 0, 1, 0, timeMode);

		// 1フレームの時間を算出。
		FbxTime framePerSecTime;
		framePerSecTime.SetTime(0, 0, 1, 0, 0, timeMode);
		// FBX の Time 値は 64bit 整数らしい。
		m_framesPerSec = static_cast<int>(framePerSecTime.Get() / m_animPeriod.Get());
	}

	void MyFbxAnimTimeInfo::AnalyzeLegacyTakeInfo(FbxScene* scene)
	{
		FbxArray<FbxString*> takeNameAry;
		//scene->FillTakeNameArray(takeNameAry); // deprecated.
		scene->FillAnimStackNameArray(takeNameAry); // なぜか const メソッドでない。
		const int takeCount = takeNameAry.GetCount();
		for (int i = 0; i < takeCount; ++i)
		{
			// FbxString::mData はゼロ終端らしい。FbxString::Buffer() も明記はされていないがゼロ終端？
			const auto strTakeName = MyUtils::SafeConvertUtf8toUtf16(takeNameAry[i]->Buffer());
			ATLTRACE(L"TakeName[%d] = \"%s\"\n", i, strTakeName.c_str());
			const auto* currentTakeInfo = scene->GetTakeInfo(*(takeNameAry[i]));
			if (currentTakeInfo)
			{
				const auto localTimeStart = currentTakeInfo->mLocalTimeSpan.GetStart();
				const auto localTimeStop = currentTakeInfo->mLocalTimeSpan.GetStop();
				const auto refTimeStart = currentTakeInfo->mReferenceTimeSpan.GetStart();
				const auto refTimeStop = currentTakeInfo->mReferenceTimeSpan.GetStop();
				const auto importOffset = currentTakeInfo->mImportOffset;
				ATLTRACE("LocalTimeStart = %I64d, LocalTimeStop = %I64d, RefTimeStart = %I64d, RefTimeStop = %I64d, ImportOffset = %d\n",
					localTimeStart.Get(), localTimeStop.Get(), refTimeStart.Get(), refTimeStop.Get(),
					importOffset.GetSecondCount());
			}
		}

		// FBX SDK 開発者はマジでスマートポインタ配列を使えよな……
		//DeleteAndClear(takeNameAry); // 旧コード。
		FbxArrayDelete(takeNameAry);
	}

} // end of namespace

