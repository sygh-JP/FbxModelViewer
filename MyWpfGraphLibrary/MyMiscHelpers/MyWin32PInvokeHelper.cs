using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Threading;

// unsafe キーワードを使ってポインタ経由でメモリを直接操作するメソッドを定義する場合、アプリケーション アセンブリとは切り離す。

// フラグ系 enum の内部型は uint などの符号なし型を指定するのがセオリー。

// P/Invoke はヘルパーアセンブリ内のみで使用し、より高レベルなラッパーのみ外部公開するようにするため、すべて internal にしようと思ったが、
// やはり直接呼び出せるようにしたほうがよい場面もあるので public にしている。


namespace MyMiscHelpers
{
	// 特定の Win32 DLL に属しているわけではないが、kernel32.dll, user32.dll, gdi32.dll で共通して使われる型や定数を定義する。
	namespace Win32Commons
	{
		[System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
		public struct RECT
		{
			public Int32 left;
			public Int32 top;
			public Int32 right;
			public Int32 bottom;

			public int Width { get { return this.right - this.left; } }
			public int Height { get { return this.bottom - this.top; } }

			public System.Windows.Int32Rect ToInt32Rect()
			{
				return new System.Windows.Int32Rect(this.left, this.top, this.Width, this.Height);
			}

			/// <summary>
			/// MFC の CRect::NormalizeRect() 同様。
			/// </summary>
			public void Normalize()
			{
				if (this.right < this.left)
				{
					MyGenericsHelper.Swap(ref this.right, ref this.left);
				}
				if (this.bottom < this.top)
				{
					MyGenericsHelper.Swap(ref this.bottom, ref this.top);
				}
			}
		}

		[System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
		public struct POINT
		{
			public Int32 x;
			public Int32 y;
		}

		[System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
		public struct SIZE
		{
			public Int32 cx;
			public Int32 cy;
		}


		[System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
		public struct WINDOWPOS
		{
			public IntPtr hwnd;
			public IntPtr hwndInsertAfter;
			public Int32 x;
			public Int32 y;
			public Int32 cx;
			public Int32 cy;
			public UInt32 flags;
		}

		public enum Win32Message : int
		{
			WM_SIZE = 0x0005,
			WM_GETMINMAXINFO = 0x0024,
			WM_WINDOWPOSCHANGING = 0x0046,
			WM_COMMAND = 0x0111,
			WM_SYSCOMMAND = 0x0112,
			WM_MOUSEMOVE = 0x0200,
			WM_LBUTTONDOWN = 0x201,
			WM_LBUTTONUP = 0x202,
			WM_RBUTTONDOWN = 0x205,
			WM_RBUTTONUP = 0x206,
			WM_MBUTTONDOWN = 0x207,
			WM_MBUTTONUP = 0x208,
			WM_MOUSEWHEEL = 0x20A,
			WM_XBUTTONDOWN = 0x20B,
			WM_XBUTTONUP = 0x20C,
			WM_SIZING = 0x0214,
		}

		public enum SystemCommandType : int
		{
			SC_MINIMIZE = 0xF020,
			SC_MAXIMIZE = 0xF030,
			SC_CLOSE = 0xF060,
			SC_RESTORE = 0xF120,
		}

		/// <summary>
		/// Win32 MessageBox() API で表示されるダイアログのコントロール ID でもある。
		/// </summary>
		public enum DialogBoxCommandIds : int
		{
			IDOK = 1,
			IDCANCEL = 2,
			IDABORT = 3,
			IDRETRY = 4,
			IDIGNORE = 5,
			IDYES = 6,
			IDNO = 7,
			IDCLOSE = 8,
			IDHELP = 9,
			IDTRYAGAIN = 10,
			IDCONTINUE = 11,
			/// <summary>
			/// 隠し ID。
			/// </summary>
			IconStatic = 14,
			/// <summary>
			/// 隠し ID。
			/// </summary>
			MainStaticTextLabel = 0xffff,
			// NOTE: 隠し ID は Windows 7 で Spy++ を使って確認しただけなので、将来的にも使えるかどうかは保証されない。
			// また、Windows 7 の MS Paint などで使われているタスク ダイアログは DirectUIHWND という特殊な
			// カスタム モーダル ダイアログになっているので、通用しない。
			// さらに、MS Office 2010 の場合も NetUIHWND という特殊なカスタム モーダル ダイアログに
			// なっていて、これもまた通用しない。
		}
	}


	/// <summary>
	/// デバイス コンテキストのハンドル HDC を IDisposable としてラップするクラス。確実な解放を保証する。
	/// HDC は 0 および -1 が無効値。
	/// ちなみに Win32 ファイル ハンドルは 0 と -1 が無効値で、HWND は -1 が無効値。
	/// ただし IsWindow(0) もエラーになる。
	/// </summary>
	public sealed class SafeHDC : Microsoft.Win32.SafeHandles.SafeHandleZeroOrMinusOneIsInvalid
	{
		private SafeHDC() : base(true) { }

		protected override bool ReleaseHandle()
		{
			bool retVal = Gdi32DllMethodsInvoker.DeleteDC(base.handle);
			//base.SetHandleAsInvalid(); // SetHandleAsInvalid() は handle に無効値を入れてくれるわけではないらしい。
			base.handle = IntPtr.Zero;
			return retVal;
		}
	}

	// CreateDC() に対応するのは DeleteDC()。
	// GetDC() や GetWindowDC() に対応するのは ReleaseDC()。
	// それぞれ実装されている DLL が異なる。

	/// <summary>
	/// 定数は WinGDI.h より抜粋。
	/// </summary>
	[System.Security.SuppressUnmanagedCodeSecurity]
	public static class Gdi32DllMethodsInvoker
	{
		public const Int32 LOGPIXELSX = 88;
		public const Int32 LOGPIXELSY = 90;

		[System.Runtime.InteropServices.DllImport("gdi32.dll", CharSet = System.Runtime.InteropServices.CharSet.Auto, ExactSpelling = true)]
		public static extern Boolean DeleteDC(IntPtr hDC);

		[System.Runtime.InteropServices.DllImport("gdi32.dll", CharSet = System.Runtime.InteropServices.CharSet.Auto, ExactSpelling = true)]
		public static extern Int32 GetDeviceCaps(IntPtr hDC, Int32 nIndex);

		[System.Runtime.InteropServices.DllImport("gdi32.dll", CharSet = System.Runtime.InteropServices.CharSet.Auto, ExactSpelling = true)]
		public static extern Int32 GetDeviceCaps(SafeHDC hDC, Int32 nIndex);

		[System.Runtime.InteropServices.DllImport("gdi32.dll", EntryPoint = "CreateDC", CharSet = System.Runtime.InteropServices.CharSet.Auto)]
		public static extern SafeHDC CreateDC(String lpszDriver, String lpszDeviceName, String lpszOutput, IntPtr devMode);

		public static SafeHDC CreateDC(String lpszDriver)
		{
			return Gdi32DllMethodsInvoker.CreateDC(lpszDriver, null, null, IntPtr.Zero);
		}

		public enum TernaryRasterOperationType : uint
		{
			SRCCOPY = 0x00CC0020,
			SRCPAINT = 0x00EE0086,
			SRCAND = 0x008800C6,
			SRCINVERT = 0x00660046,
			SRCERASE = 0x00440328,
			NOTSRCCOPY = 0x00330008,
			NOTSRCERASE = 0x001100A6,
			MERGECOPY = 0x00C000CA,
			MERGEPAINT = 0x00BB0226,
			PATCOPY = 0x00F00021,
			PATPAINT = 0x00FB0A09,
			PATINVERT = 0x005A0049,
			DSTINVERT = 0x00550009,
			BLACKNESS = 0x00000042,
			WHITENESS = 0x00FF0062,
			CAPTUREBLT = 0x40000000,
		}

		[System.Runtime.InteropServices.DllImport("gdi32.dll")]
		[return: System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.Bool)]
		public static extern bool BitBlt(IntPtr hdc, Int32 nXDest, Int32 nYDest, Int32 nWidth, Int32 nHeight,
			IntPtr hdcSrc, Int32 nXSrc, Int32 nYSrc, TernaryRasterOperationType dwRop);
	}

	/// <summary>
	/// 定数は WinUser.h より抜粋。
	/// </summary>
	public static class User32DllMethodsInvoker
	{
		public enum IndexOfGetWindowLong : int
		{
			GWL_WNDPROC = (-4),
			GWL_HINSTANCE = (-6),
			GWL_HWNDPARENT = (-8),
			GWL_ID = (-12),
			GWL_STYLE = (-16),
			GWL_EXSTYLE = (-20),
			GWL_USERDATA = (-21),
		}

		public enum IndexOfGetClassLong : int
		{
			GCL_MENUNAME = (-8),
			GCL_HBRBACKGROUND = (-10),
			GCL_HCURSOR = (-12),
			GCL_HICON = (-14),
			GCL_HMODULE = (-16),
			GCL_CBWNDEXTRA = (-18),
			GCL_CBCLSEXTRA = (-20),
			GCL_WNDPROC = (-24),
			GCL_STYLE = (-26),
			GCW_ATOM = (-32),
		}

		[Flags]
		public enum WindowStyles : uint
		{
			WS_OVERLAPPED = 0x00000000,
			WS_POPUP = 0x80000000,
			WS_CHILD = 0x40000000,
			WS_MINIMIZE = 0x20000000,
			WS_VISIBLE = 0x10000000,
			WS_DISABLED = 0x08000000,
			WS_CLIPSIBLINGS = 0x04000000,
			WS_CLIPCHILDREN = 0x02000000,
			WS_MAXIMIZE = 0x01000000,
			WS_BORDER = 0x00800000,
			WS_DLGFRAME = 0x00400000,
			WS_VSCROLL = 0x00200000,
			WS_HSCROLL = 0x00100000,
			WS_SYSMENU = 0x00080000,
			WS_THICKFRAME = 0x00040000,
			WS_GROUP = 0x00020000,
			WS_TABSTOP = 0x00010000,

			WS_MINIMIZEBOX = 0x00020000,
			WS_MAXIMIZEBOX = 0x00010000,

			WS_CAPTION = WS_BORDER | WS_DLGFRAME,
			WS_TILED = WS_OVERLAPPED,
			WS_ICONIC = WS_MINIMIZE,
			WS_SIZEBOX = WS_THICKFRAME,
			WS_TILEDWINDOW = WS_OVERLAPPEDWINDOW,

			WS_OVERLAPPEDWINDOW = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
			WS_POPUPWINDOW = WS_POPUP | WS_BORDER | WS_SYSMENU,
			WS_CHILDWINDOW = WS_CHILD,

			DS_MODALFRAME = 0x80,
		}

		[Flags]
		public enum ExWindowStyles : uint
		{
			WS_EX_TRANSPARENT = 0x00000020,
			WS_EX_LAYERED = 0x00080000,
			WS_EX_COMPOSITED = 0x02000000,
		}

		[Flags]
		public enum LayeredWindowAttributes : uint
		{
			LWA_COLORKEY = 0x00000001,
			LWA_ALPHA = 0x00000002,
		}

		[Flags]
		public enum MenuFlags : uint
		{
			MF_BYCOMMAND = 0x000,
			MF_BYPOSITION = 0x400,
			MF_ENABLED = 0x00,
			MF_GRAYED = 0x01,
			MF_DISABLED = 0x02,
			MF_REMOVE = 0x1000,
		}

		public enum SystemParametersInfoType : uint
		{
			SPI_GETWORKAREA = 0x0030,
		}

		/// <summary>
		/// ShowWindow(), ShowWindowAsync() 関数のパラメータに渡す定数。
		/// WINDOWPLACEMENT 構造体メンバーにも使われている。
		/// </summary>
		public enum CommandOfShowWindow : uint
		{
			SW_HIDE = 0,
			SW_SHOWNORMAL = 1,
			SW_NORMAL = 1,
			SW_SHOWMINIMIZED = 2,
			SW_SHOWMAXIMIZED = 3,
			SW_MAXIMIZE = 3,
			SW_SHOWNOACTIVATE = 4,
			SW_SHOW = 5,
			SW_MINIMIZE = 6,
			SW_SHOWMINNOACTIVE = 7,
			SW_SHOWNA = 8,
			SW_RESTORE = 9,
			SW_SHOWDEFAULT = 10,
			SW_FORCEMINIMIZE = 11,
			SW_MAX = 11,
		}

		public enum CommandOfGetWindow : uint
		{
			GW_HWNDFIRST = 0,
			GW_HWNDLAST = 1,
			GW_HWNDNEXT = 2,
			GW_HWNDPREV = 3,
			GW_OWNER = 4,
			GW_CHILD = 5,
			GW_ENABLEDPOPUP = 6,
		}

		public enum MonitorFlagType : uint
		{
			MONITOR_DEFAULTTONULL = 0,
			MONITOR_DEFAULTTOPRIMARY = 1,
			MONITOR_DEFAULTTONEAREST = 2,
		}

		[Flags]
		public enum MenuItemInfoMaskType : uint
		{
			MIIM_BITMAP = 0x00000080,
			MIIM_CHECKMARKS = 0x00000008,
			MIIM_DATA = 0x00000020,
			MIIM_FTYPE = 0x00000100,
			MIIM_ID = 0x00000002,
			MIIM_STATE = 0x00000001,
			MIIM_STRING = 0x00000040,
			MIIM_SUBMENU = 0x00000004,
			MIIM_TYPE = 0x00000010,
		}

		[Flags]
		public enum MouseKeyFlags : uint
		{
			MK_LBUTTON = 0x0001,
			MK_RBUTTON = 0x0002,
			MK_SHIFT = 0x0004,
			MK_CONTROL = 0x0008,
			MK_MBUTTON = 0x0010,
			MK_XBUTTON1 = 0x0020,
			MK_XBUTTON2 = 0x0040,
		}

		public enum WindowsHookType : int
		{
			WH_JOURNALRECORD = 0,
			WH_JOURNALPLAYBACK = 1,
			WH_KEYBOARD = 2,
			WH_GETMESSAGE = 3,
			WH_CALLWNDPROC = 4,
			WH_CBT = 5,
			WH_SYSMSGFILTER = 6,
			WH_MOUSE = 7,
			WH_HARDWARE = 8,
			WH_DEBUG = 9,
			WH_SHELL = 10,
			WH_FOREGROUNDIDLE = 11,
			WH_CALLWNDPROCRET = 12,
			WH_KEYBOARD_LL = 13,
			WH_MOUSE_LL = 14,
		}

		public enum HookCode : int
		{
			HC_ACTION = 0,
			HC_GETNEXT = 1,
			HC_SKIP = 2,
			HC_NOREMOVE = 3,
			HC_NOREM = HC_NOREMOVE,
			HC_SYSMODALON = 4,
			HC_SYSMODALOFF = 5,
		}

		[System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
		public struct MONITORINFO
		{
			public UInt32 cbSize;
			public Win32Commons.RECT rcMonitor;
			public Win32Commons.RECT rcWork;
			public UInt32 dwFlags;

			public void InitializeSize()
			{
				this.cbSize = (UInt32)System.Runtime.InteropServices.Marshal.SizeOf(this);
			}
		}

		[System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
		public struct WINDOWPLACEMENT
		{
			public UInt32 length;
			public UInt32 flags;
			public CommandOfShowWindow showCmd;
			public Win32Commons.POINT ptMinPosition;
			public Win32Commons.POINT ptMaxPosition;
			public Win32Commons.RECT rcNormalPosition;

			public void InitializeLength()
			{
				this.length = (UInt32)System.Runtime.InteropServices.Marshal.SizeOf(this);
			}
		}

		[System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
		public struct MINMAXINFO
		{
			public Win32Commons.POINT ptReserved;
			/// <summary>
			/// ウィンドウが最大化されるときの、ウィンドウの幅 (point.x) と高さ (point.y) を指定します。
			/// </summary>
			public Win32Commons.POINT ptMaxSize;
			/// <summary>
			/// 最大化されるときの、ウィンドウの左辺の位置 (point.x) と上辺の位置 (point.y) を指定します。
			/// </summary>
			public Win32Commons.POINT ptMaxPosition;
			/// <summary>
			/// ウィンドウの最小トラッキングの幅 (point.x) と最小トラッキングの高さ (point.y) を指定します。
			/// </summary>
			public Win32Commons.POINT ptMinTrackSize;
			/// <summary>
			/// ウィンドウの最大トラッキングの幅 (point.x) と最大トラッキングの高さ (point.y) を指定します。
			/// </summary>
			public Win32Commons.POINT ptMaxTrackSize;
		}

		[System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
		public struct MSLLHOOKSTRUCT
		{
			public Win32Commons.POINT pt;
			public Int32 mouseData; // be careful, this must be ints, not uints (was wrong before I changed it...). regards, cmew.
			public Int32 flags;
			public Int32 time;
			public UIntPtr dwExtraInfo;
		}

		// CharSet の指定なしだと Ansi 扱いになってしまう模様。メソッドのほうも同様。
		[System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
		public struct MENUITEMINFO
		{
			public UInt32 cbSize;
			public MenuItemInfoMaskType fMask;
			public UInt32 fType;
			public UInt32 fState;
			public Int32 wID;
			public IntPtr hSubMenu;
			public IntPtr hbmpChecked;
			public IntPtr hbmpUnchecked;
			public UIntPtr dwItemData;
			public string dwTypeData;
			public UInt32 cch;
			public IntPtr hbmpItem;
		}

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern Int32 GetWindowLong(IntPtr hWnd, Int32 index);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern Int32 SetWindowLong(IntPtr hWnd, Int32 index, Int32 newStyle);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern UInt32 GetClassLong(IntPtr hWnd, Int32 index);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern UInt32 SetClassLong(IntPtr hWnd, Int32 index, Int32 newLong);

		// Win64 には GetWindowLongPtr() / SetWindowLongPtr() の関数エントリーポイントが実際に存在するが、
		// Win32 ではマクロ実装で GetWindowLong() / SetWindowLong() に置き換わるだけなのでエントリーポイントを取得できないことに注意。
		// P/Invoke は基本的に遅延バインディングなので、extern 対象の DLL に関数エントリーポイントが存在していなくても実際に呼び出さなければ問題ない。
		// ただし内部的には GetProcAddress() を使ってエントリーポイントを検索する処理が毎回走っている可能性があるので、
		// C/C++ で直接呼び出す場合と比べてオーバーヘッドがあるものと思われる。
		// なお、ウィンドウ スタイル系フラグは 32bit 範囲分しか使われていないので、Win32/Win64 で呼び出しを切り替える必要はない。
		// 64bit 版で Ptr 系を使う状況というのは、ポインタ値をウィンドウにバインドする必要があるときのみ。

		#region Win64 Only
		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern IntPtr GetWindowLongPtr(IntPtr hWnd, Int32 index);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern IntPtr SetWindowLongPtr(IntPtr hWnd, Int32 index, IntPtr newStyle);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern UIntPtr GetClassLongPtr(IntPtr hWnd, Int32 index);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern UIntPtr SetClassLongPtr(IntPtr hWnd, Int32 index, IntPtr newLong);
		#endregion

		#region 高レベル ラッパー。
		public static WindowStyles GetWindowStyle(IntPtr hWnd)
		{
			return (WindowStyles)GetWindowLong(hWnd, (Int32)IndexOfGetWindowLong.GWL_STYLE);
		}

		public static void SetWindowStyle(IntPtr hWnd, WindowStyles windowStyle)
		{
			SetWindowLong(hWnd, (Int32)IndexOfGetWindowLong.GWL_STYLE, (Int32)windowStyle);
		}

		public static ExWindowStyles GetWindowStyleEx(IntPtr hWnd)
		{
			return (ExWindowStyles)GetWindowLong(hWnd, (Int32)IndexOfGetWindowLong.GWL_EXSTYLE);
		}

		public static void SetWindowStyleEx(IntPtr hWnd, ExWindowStyles windowStyle)
		{
			SetWindowLong(hWnd, (Int32)IndexOfGetWindowLong.GWL_EXSTYLE, (Int32)windowStyle);
		}
		#endregion

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern bool SetForegroundWindow(IntPtr hWnd);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern bool ShowWindow(IntPtr hWnd, CommandOfShowWindow nCmdShow);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern bool ShowWindowAsync(IntPtr hWnd, CommandOfShowWindow nCmdShow);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern bool IsIconic(IntPtr hWnd);

		[System.Runtime.InteropServices.DllImport("user32.dll", SetLastError = true)]
		public static extern bool SetLayeredWindowAttributes(IntPtr hWnd, UInt32 crKey, byte bAlpha, LayeredWindowAttributes dwFlags);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern bool PrintWindow(IntPtr hwnd, IntPtr hDC, UInt32 nFlags);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern bool GetWindowRect(IntPtr hwnd, ref Win32Commons.RECT lpRect);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern bool GetClientRect(IntPtr hwnd, ref Win32Commons.RECT lpRect);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern bool ClientToScreen(IntPtr hWnd, ref Win32Commons.POINT lpPoint);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern bool ScreenToClient(IntPtr hWnd, ref Win32Commons.POINT lpPoint);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern IntPtr WindowFromPoint(Win32Commons.POINT point);

		public delegate bool EnumWindowsProcDelegate(IntPtr hWnd, IntPtr lParam);
		public delegate bool EnumChildWindowsProcDelegate(IntPtr hWnd, IntPtr lParam);
		public delegate bool EnumThreadWindowsProcDelegate(IntPtr hWnd, IntPtr lParam);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern bool EnumWindows(EnumWindowsProcDelegate lpEnumFunc, IntPtr lParam);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern bool EnumChildWindows(IntPtr hWndParent, EnumChildWindowsProcDelegate lpEnumFunc, IntPtr lParam);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern bool EnumThreadWindows(uint dwThreadId, EnumChildWindowsProcDelegate lpEnumFunc, IntPtr lParam);

		[System.Runtime.InteropServices.DllImport("user32.dll", CharSet = System.Runtime.InteropServices.CharSet.Auto)]
		public static extern IntPtr FindWindow(string lpClassName, string lpWindowName);

		[System.Runtime.InteropServices.DllImport("user32.dll", CharSet = System.Runtime.InteropServices.CharSet.Auto)]
		public static extern IntPtr FindWindowEx(IntPtr hwndParent, IntPtr hwndChildAfter, string lpszClass, string lpszWindow);

		[System.Runtime.InteropServices.DllImport("user32.dll", CharSet = System.Runtime.InteropServices.CharSet.Auto)]
		public static extern Int32 GetClassName(IntPtr hWnd, StringBuilder lpClassName, Int32 nMaxCount);

		public delegate IntPtr HookProcDelegate(Int32 code, IntPtr wParam, IntPtr lParam);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern IntPtr SetWindowsHookEx(WindowsHookType hookType, HookProcDelegate lpfn, IntPtr hMod, UInt32 dwThreadId);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern IntPtr CallNextHookEx(IntPtr hhk, Int32 nCode, IntPtr wParam, IntPtr lParam);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern bool UnhookWindowsHookEx(IntPtr hhk);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern bool IsWindowVisible(IntPtr hWnd);

		[System.Runtime.InteropServices.DllImport("user32.dll", CharSet = System.Runtime.InteropServices.CharSet.Auto)]
		public static extern Int32 GetWindowText(IntPtr hWnd, StringBuilder lpString, Int32 nMaxCount);

		[System.Runtime.InteropServices.DllImport("user32.dll", SetLastError = true, CharSet = System.Runtime.InteropServices.CharSet.Auto)]
		public static extern Int32 GetWindowTextLength(IntPtr hWnd);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern bool IsWindow(IntPtr hWnd);

		[System.Runtime.InteropServices.DllImport("user32.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
		public static extern IntPtr SendMessage(IntPtr hWnd, Win32Commons.Win32Message Msg, IntPtr wParam, IntPtr lParam);

		[System.Runtime.InteropServices.DllImport("user32.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
		public static extern bool PostMessage(IntPtr hWnd, Win32Commons.Win32Message Msg, IntPtr wParam, IntPtr lParam);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern UInt32 GetWindowThreadProcessId(IntPtr hWnd, out UInt32 lpdwProcessId);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern UInt32 GetWindowThreadProcessId(IntPtr hWnd, IntPtr lpdwProcessId);

		[System.Runtime.InteropServices.DllImport("user32.dll", SetLastError = true)]
		public static extern IntPtr GetWindow(IntPtr hWnd, CommandOfGetWindow uCmd);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern IntPtr GetParent(IntPtr hWnd);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern IntPtr GetDesktopWindow();

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern IntPtr GetDlgItem(IntPtr hDlg, Int32 nIDDlgItem);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern IntPtr GetWindowDC(IntPtr hWnd);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern Int32 ReleaseDC(IntPtr hWnd, IntPtr hDC);

		[System.Runtime.InteropServices.DllImport("user32.dll", SetLastError = true)]
		public static extern bool SystemParametersInfo(SystemParametersInfoType uiAction, UInt32 uiParam, IntPtr pvParam, UInt32 fWinIni);

		[System.Runtime.InteropServices.DllImport("user32.dll", SetLastError = true)]
		public static extern bool SystemParametersInfo(SystemParametersInfoType uiAction, UInt32 uiParam, ref Win32Commons.RECT pvParam, UInt32 fWinIni);

		// P/Invoke でジェネリクスは使えない。明示的にオーバーロードを定義してやる必要がある。

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		[return: System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.Bool)]
		public static extern bool GetWindowPlacement(IntPtr hWnd, ref WINDOWPLACEMENT lpwndpl);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern IntPtr MonitorFromWindow(IntPtr hwnd, MonitorFlagType dwFlags);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		[return: System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.Bool)]
		public static extern bool GetMonitorInfo(IntPtr hMonitor, ref MONITORINFO lpmi);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern IntPtr GetSystemMenu(IntPtr hWnd, bool bRevert);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern Int32 GetMenuItemCount(IntPtr hMenu);

		[System.Runtime.InteropServices.DllImport("user32.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
		public static extern bool GetMenuItemInfo(IntPtr hMenu, UInt32 uItem, bool fByPosition, ref MENUITEMINFO lpmii);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern bool DrawMenuBar(IntPtr hWnd);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern bool RemoveMenu(IntPtr hMenu, UInt32 uPosition, MenuFlags uFlags);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		public static extern bool EnableMenuItem(IntPtr hMenu, UInt32 uIDEnableItem, MenuFlags uEnable);
	}

	public static class Kernel32DllMethodsInvoker
	{
		// NOTE: GetLastError() に関しては、System.Runtime.InteropServices.Marshal.GetLastWin32Error() が用意されている。

		[System.Runtime.InteropServices.DllImport("kernel32.dll", SetLastError = true)]
		public static extern bool CloseHandle(IntPtr hObject);

		// System.Runtime.InteropServices.Marshal.Copy() ではサポートされない。

		[System.Runtime.InteropServices.DllImport("kernel32.dll")]
		public static extern void CopyMemory(IntPtr dst, IntPtr src, IntPtr size);

		[System.Runtime.InteropServices.DllImport("Kernel32.dll", EntryPoint = "RtlZeroMemory")]
		public static extern void ZeroMemory(IntPtr dest, IntPtr size);

		[System.Runtime.InteropServices.DllImport("kernel32.dll")]
		public static extern bool QueryFullProcessImageName(IntPtr hProcess, int dwFlags, [System.Runtime.InteropServices.Out] StringBuilder lpExeName, ref int lpdwSize);
	}

	public static class ShellWApiDllMethodsInvoker
	{
		// Windows エクスプローラーでファイルを名前ソートする際、
		// "1.txt" と "10.txt" の順序が機械的な序数順（Ordinal）ではなく自然順（Logical, Natural Sort）になっているのは、
		// この API を内部で使用しているためと思われる。
		// .NET 基本クラス ライブラリの System.String のメソッドおよびオプションには、直接該当するものはなさげ。
		[System.Runtime.InteropServices.DllImport("shlwapi.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
		public static extern Int32 StrCmpLogicalW(string psz1, string psz2);
	}
}
