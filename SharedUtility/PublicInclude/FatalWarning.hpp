#pragma once

// 無視すべきでない警告をエラー扱いにする。

#ifdef _MSC_VER
#pragma warning(error : 4715) // 「値を返さないコントロール パスがあります。」をエラーにする。
#endif
