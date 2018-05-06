#pragma once

#include "MyUtil.h"


namespace MyFbx
{
	// FBX にはスケルトンの他にクラスタというボーン関連の概念があるが、階層構造を管理するのはスケルトン。
	// ちなみに BodyPaint 3D のマニュアルによると、CINEMA 4D のボーンは 2 個の連続した FBX スケルトン ノードに相当するらしいが、
	// おそらく一般的な他の 3D ツールも同様のはず。

	class SkeletonInfo final : boost::noncopyable
	{
	public:
		typedef std::shared_ptr<SkeletonInfo> TSharedPtr;
		typedef std::shared_ptr<const SkeletonInfo> TConstSharedPtr;
	public:
		SkeletonInfo()
			: m_parent()
		{
		}

#if 0
		virtual ~SkeletonInfo() {} // HACK: 継承しないことが前提であれば、オーバーヘッドの大きい public 仮想にする必要は無い。
#endif

		// 子スケルトン追加
		void AddChild(TSharedPtr child) { m_children.push_back(child); }

		const std::vector<TSharedPtr>& GetChildren() const { return m_children; }

		// 親スケルトン登録
		void SetParent(SkeletonInfo* parent) { m_parent = parent; }
		SkeletonInfo* GetParent() { return m_parent; }
		const SkeletonInfo* GetParent() const { return m_parent; }

		bool GetIsRoot() const { return m_parent == nullptr; }
		bool GetIsLeaf() const { return m_children.empty(); }

		void SetSkeletonName(const char* pName) { m_name = MyUtils::SafeConvertUtf8toUtf16(pName); }
		void SetSkeletonName(const wchar_t* pName) { m_name = pName ? pName : L""; }

		const std::wstring& GetSkeletonNameW() const { return m_name; }

	private:
		std::wstring m_name; // スケルトン名。
		SkeletonInfo* m_parent; // 親スケルトンへのポインタ。子をスマートポインタで管理しているので、循環参照防止のため親にはスマートポインタは使わない。
		std::vector<TSharedPtr> m_children; // 子ボーン達。
	};

	// shared_ptr による循環参照は防止する必要があるが、生ポインタはどうしても使いたくない場合、std::weak_ptr と組み合わせる方法もある。


	//typedef std::map<std::wstring, SkeletonInfo::TSharedPtr> TStrToSkeletonInfoPtrMap;
	typedef std::unordered_map<std::wstring, SkeletonInfo::TSharedPtr> TStrToSkeletonInfoPtrMap;


	class MyFbxSkeletonAnalyzer
	{
	public:
		typedef std::shared_ptr<MyFbxSkeletonAnalyzer> TSharedPtr;
		typedef std::shared_ptr<const MyFbxSkeletonAnalyzer> TConstSharedPtr;
	public:
		MyFbxSkeletonAnalyzer();

		virtual ~MyFbxSkeletonAnalyzer();

		// 解析
		void Analyze(FbxNode* node, const FbxSkeleton* skeleton);

	public:
		//! @brief  スケルトンを名前で検索する。<br>
		const SkeletonInfo* FindSkeleton(const std::wstring& skeletonName) const;
		SkeletonInfo* FindSkeleton(const std::wstring& skeletonName)
		{ return const_cast<SkeletonInfo*>((static_cast<const MyFbxSkeletonAnalyzer*>(this))->FindSkeleton(skeletonName)); }
		// 非 const 版の実装に関しては、Effecttive C++ を参照のこと。

		const TStrToSkeletonInfoPtrMap& GetSkeletonMap() const { return m_skeletonInfoMap; }
		//SkeletonInfo::TSharedPtr GetRootSkeleton() const { return m_rootSkeleton; }

	private:
		TStrToSkeletonInfoPtrMap m_skeletonInfoMap; // スケルトン マップ。名前をキーとする。
		//SkeletonInfo::TSharedPtr m_rootSkeleton; // ルートスケルトン
	};

}
