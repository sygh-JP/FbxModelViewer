#pragma once

// boost::noncopyable だと Empty Base Optimization が効かない。
// MSVC でも Clang でも同様。

namespace MyUtils
{

	//! @biref  C++11 による noncopyable 基底クラス。ムーブ コンストラクタのほうはとりあえず放置。<br>
	//! クラス テンプレートにすることで Empty Base Optimization が効くようになる。<br>
	template <typename T>
	struct MyNoncopyable
	{
		MyNoncopyable() = default;
		MyNoncopyable(const MyNoncopyable&) = delete;
		MyNoncopyable& operator=(const MyNoncopyable&) = delete;
	};

} // end of namespace
