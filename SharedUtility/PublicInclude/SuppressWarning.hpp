#pragma once

// 無視しても問題ない警告を抑制する。


#ifdef _MSC_VER
#if (_MSC_VER >= 1400)
#pragma warning (disable : 4351) // 「新しい動作: 配列 'XXX' の要素は既定で初期化されます」を抑制する。
#else
#error Please do not use older compiler than Visual C++ 2005!
#endif
#endif
