#include "stdafx.h"

#include "FbxSkeletonAnalyzer.h"


namespace MyFbx
{


	MyFbxSkeletonAnalyzer::MyFbxSkeletonAnalyzer()
	{
	}

	MyFbxSkeletonAnalyzer::~MyFbxSkeletonAnalyzer()
	{
	}

	void MyFbxSkeletonAnalyzer::Analyze(FbxNode* node, const FbxSkeleton* skeleton)
	{
		// スケルトンタイプ取得
		const auto type = skeleton->GetSkeletonType();

		_ASSERTE(node != nullptr);
		const auto nodeName = MyUtils::ConvertUtf8toUtf16(node->GetName());

		// もしノードだったら接続をチェックする
		if (type == FbxSkeleton::eLimbNode)
		{
			SkeletonInfo::TSharedPtr mySkeletonInfo(new SkeletonInfo());
			mySkeletonInfo->SetSkeletonName(nodeName.c_str());

			// 親ノード検索。
			const auto* parentNode = node->GetParent();
			_ASSERTE(parentNode != nullptr);
			const auto parentName = MyUtils::ConvertUtf8toUtf16(parentNode->GetName());
			auto parentSkeleton = this->FindSkeleton(parentName);
			if (parentSkeleton == nullptr)
			{
				// ルートスケルトンと判断。
				//m_rootSkeleton = mySkeletonInfo;
			}
			else
			{
				// 親スケルトンと連結。
				parentSkeleton->AddChild(mySkeletonInfo);

				// 親スケルトンを登録。
				mySkeletonInfo->SetParent(parentSkeleton);
			}
			// マップへ登録。
			m_skeletonInfoMap.insert(std::make_pair(mySkeletonInfo->GetSkeletonNameW(), mySkeletonInfo));
		}
		else if (type == FbxSkeleton::eRoot)
		{
			SkeletonInfo::TSharedPtr mySkeletonInfo(new SkeletonInfo());
			mySkeletonInfo->SetSkeletonName(nodeName.c_str());

			// 例えば FBX Converter 付属の LocalMotionBlend.fbx には、
			// 少なくとも3つ（"PlasticMan:Reference", "PlasticMan:Hips", "PlasticMan_Ctrl:Reference"）のルートノードが含まれる。
			// また、"PlasticMan:Hips" は種別がルートノードでありながら "PlasticMan:Reference" を親に持つ。
			// その対処が必要。
			ATLTRACE(__FUNCTIONW__ L"() Skeleton Root: '%s'\n", nodeName.c_str());

#if 1
			// 親ノード検索。
			const auto* parentNode = node->GetParent();
			_ASSERTE(parentNode != nullptr);
			const auto parentName = MyUtils::ConvertUtf8toUtf16(parentNode->GetName());
			auto parentSkeleton = this->FindSkeleton(parentName);
			if (parentSkeleton == nullptr)
			{
				// ルートスケルトンと判断。
				//m_rootSkeleton = mySkeletonInfo;
			}
			else
			{
				// 親スケルトンと連結。
				parentSkeleton->AddChild(mySkeletonInfo);

				// 親スケルトンを登録。
				mySkeletonInfo->SetParent(parentSkeleton);
			}
#endif
			// マップへ登録。
			m_skeletonInfoMap.insert(std::make_pair(mySkeletonInfo->GetSkeletonNameW(), mySkeletonInfo));
		}
		else if (type == FbxSkeleton::eLimb)
		{
			ATLTRACE(__FUNCTIONW__ L"() Skeleton Limb: '%s'\n", nodeName.c_str());
		}
		else if (type == FbxSkeleton::eEffector)
		{
			ATLTRACE(__FUNCTIONW__ L"() Skeleton Effector: '%s'\n", nodeName.c_str());
		}
	}

	const SkeletonInfo* MyFbxSkeletonAnalyzer::FindSkeleton(const std::wstring& skeletonName) const
	{
		auto it = m_skeletonInfoMap.find(skeletonName);
		if (it != m_skeletonInfoMap.end())
		{
			return it->second.get();
		}
		else
		{
			return nullptr;
		}
	}
}
