#pragma once

namespace MyUtil
{
	typedef std::vector<int32_t> TIntArray;
	typedef std::vector<uint32_t> TUIntArray;
	typedef std::vector<std::wstring> TStrArray;

	//typedef std::pair<int32_t, int32_t> TIntIntPair;
	//typedef std::map<int32_t, int32_t> TIntToIntMap;
	//typedef std::map<std::wstring, int32_t> TStrToIntMap;

	typedef std::unordered_map<int32_t, int32_t> TIntToIntMap;
	typedef std::unordered_map<std::wstring, int32_t> TStrToIntMap;

} // end of namespace


using MyUtil::TIntArray;
using MyUtil::TUIntArray;
using MyUtil::TStrArray;
//using MyUtil::TIntIntPair;
using MyUtil::TIntToIntMap;
using MyUtil::TStrToIntMap;
