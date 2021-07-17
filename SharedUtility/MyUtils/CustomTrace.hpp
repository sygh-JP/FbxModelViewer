#pragma once


#pragma region // #pragma message() などで使ってください。//
// http://support.microsoft.com/kb/155196/en-us
// https://web.archive.org/web/20101121150327/http://support.microsoft.com/kb/155196
// https://docs.microsoft.com/en-us/cpp/preprocessor/message

//! @brief  マクロ仮引数の値をダブルクォーテーションを付けた状態に置き換えます。（s → L"s"）<br>
#define MY_SYMBOL_TO_STRINGW(s)  L#s
//! @brief  マクロ仮引数の値をダブルクォーテーションを付けた状態に置き換えます。（s → "s"）<br>
#define MY_SYMBOL_TO_STRINGA(s)  #s

#define MY_DEFMACRO_TO_STRINGW(s)  MY_SYMBOL_TO_STRINGW(s)
#define MY_DEFMACRO_TO_STRINGA(s)  MY_SYMBOL_TO_STRINGA(s)


//! @brief  リテラル ASCII 文字列をリテラル ワイド文字列に置き換える。<br>
#define MY_LITERAL_STRINGA_TO_STRINGW(s)  L##s
//! @brief  リテラル ASCII 文字列のマクロをリテラル ワイド文字列に置き換える。<br>
#define MY_DEFMACRO_OF_LITERAL_STRINGA_TO_STRINGW(s)  MY_LITERAL_STRINGA_TO_STRINGW(s)


#pragma region // IDE に出力されたトレース行をダブルクリックすることで、該当コードへジャンプできるようになります。//

#define MY_LOC_FOR_MESSAGE_TO_STRINGA    __FILE__ "(" MY_DEFMACRO_TO_STRINGA(__LINE__) ") : "
#define MY_LOC_FOR_MESSAGE_TO_STRINGW    MY_DEFMACRO_OF_LITERAL_STRINGA_TO_STRINGW(__FILE__) L"(" MY_DEFMACRO_TO_STRINGW(__LINE__) L") : "

// __FILEW__ のほかに __FUNCTIONW__ も存在するが、標準規格ではないので注意。

#define MY_LOC_FOR_WARNING_TO_STRINGA    MY_LOC_FOR_MESSAGE_TO_STRINGA  "Custom Warning : "
#define MY_LOC_FOR_WARNING_TO_STRINGW    MY_LOC_FOR_MESSAGE_TO_STRINGW L"Custom Warning : "

#pragma endregion


#ifdef _UNICODE
#define MY_SYMBOL_TO_STRINGT(s)  L#s
#define MY_DEFMACRO_TO_STRINGT(s)  MY_SYMBOL_TO_STRINGW(s)
#define MY_LOC_FOR_MESSAGE_TO_STRINGT  MY_LOC_FOR_MESSAGE_TO_STRINGW
#define MY_LOC_FOR_WARNING_TO_STRINGT  MY_LOC_FOR_WARNING_TO_STRINGW
#else
#define MY_SYMBOL_TO_STRINGT(s)  #s
#define MY_DEFMACRO_TO_STRINGT(s)  MY_SYMBOL_TO_STRINGA(s)
#define MY_LOC_FOR_MESSAGE_TO_STRINGT  MY_LOC_FOR_MESSAGE_TO_STRINGA
#define MY_LOC_FOR_WARNING_TO_STRINGT  MY_LOC_FOR_WARNING_TO_STRINGA
#endif


#pragma endregion
