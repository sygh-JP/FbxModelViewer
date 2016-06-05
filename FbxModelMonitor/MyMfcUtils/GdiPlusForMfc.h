#pragma once

// MFC アプリケーションのデバッグビルドで GDI+ のクラスオブジェクトを new するとき、下記のエラーが出るのを防止する。
// error C2660: 'Gdiplus::GdiplusBase::operator new' : 関数に 3 個の引数を指定できません。

#define iterator _iterator

#ifdef _DEBUG

namespace Gdiplus
{
	namespace DllExports
	{
#include <GdiplusMem.h>
	};

#ifndef _GDIPLUSBASE_H
#define _GDIPLUSBASE_H
	class GdiplusBase
	{
	public:
		void (operator delete)(void* in_pVoid)
		{
			DllExports::GdipFree(in_pVoid);
		}

		void* (operator new)(size_t in_size)
		{
			return DllExports::GdipAlloc(in_size);
		}

		void (operator delete[])(void* in_pVoid)
		{
			DllExports::GdipFree(in_pVoid);
		}

		void* (operator new[])(size_t in_size)
		{
			return DllExports::GdipAlloc(in_size);
		}

		void * (operator new)(size_t nSize, LPCSTR lpszFileName, int nLine)
		{
			return DllExports::GdipAlloc(nSize);
		}

		void operator delete(void* p, LPCSTR lpszFileName, int nLine)
		{
			DllExports::GdipFree(p);
		}

		void * (operator new)(size_t nSize, int nType, LPCSTR lpszFileName, int nLine)
		{
			return DllExports::GdipAlloc(nSize);
		}

	};
#endif // #ifndef _GDIPLUSBASE_H
}
#endif // #ifdef _DEBUG

#include <gdiplus.h>
#undef iterator
