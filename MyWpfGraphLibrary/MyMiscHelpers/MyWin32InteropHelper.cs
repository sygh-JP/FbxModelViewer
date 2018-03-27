using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
//using System.Windows;
//using System.Windows.Threading;

// System.Windows 配下のクラスはすべて WPF 関連のものであることに注意。WinForms や WinRT とは別物で、類似性はあるが互換性はない。
// System.Windows.Threading 名前空間は WindowsBase アセンブリに属する。
// System.Windows.Interop 名前空間は PresentationCore アセンブリに属する。


namespace MyMiscHelpers
{

	internal class MyDeviceHelper
	{
#if false
		/// <summary>
		/// System.Windows.Controls.Orientation と同じ。
		/// </summary>
		public enum ScanOrientation
		{
			Horizontal,
			Vertical,
		}
#endif

		/// <summary>
		/// Windows OS の標準 DPI は 96。旧 Mac だと 72 になる。Retina ディスプレイは世代や製品によって複数ある。
		/// ちなみに Win32 API のヘッダーにはこのデフォルト数値に対するシンボルは特に定義されていない模様。
		/// Direct2D のヘルパーにも 96.0f という即値が直接埋め込まれている。
		/// </summary>
		public const int DefaultDpi = 96;

		public static int GetDisplayPixelsPerInchX()
		{
			return GetDisplayPixelsPerInch(Gdi32DllMethodsInvoker.LOGPIXELSX);
		}

		public static int GetDisplayPixelsPerInchY()
		{
			return GetDisplayPixelsPerInch(Gdi32DllMethodsInvoker.LOGPIXELSY);
		}

		private static int GetDisplayPixelsPerInch(int capIndex)
		{
			using (var handle = Gdi32DllMethodsInvoker.CreateDC("DISPLAY"))
			{
				return (handle.IsInvalid ? DefaultDpi : Gdi32DllMethodsInvoker.GetDeviceCaps(handle, capIndex));
			}
		}
	}

#if false
	[Obsolete]
	public static class MyBitHelper
	{
		public static uint LoWord(IntPtr ptr) { return ((uint)ptr) & 0xffff; }
		public static uint HiWord(IntPtr ptr) { return (((uint)ptr) >> 16) & 0xffff; }
		public static uint LoWord(UIntPtr ptr) { return ((uint)ptr) & 0xffff; }
		public static uint HiWord(UIntPtr ptr) { return (((uint)ptr) >> 16) & 0xffff; }
	}
#endif

	public static class MyBitOpHelper
	{
		#region // intsafe.h もしくは minwindef.h から移植。オリジナルは C/C++ マクロ。
		// C# デフォルトでは HiWord(), HiByte() の計算にマスク演算は不要で、キャストだけでも十分だが、
		// オーバーフローをチェックする設定の場合でも無視できるようにする。

		public static ushort LoWord(int val) { return LoWord((uint)val); }
		public static ushort HiWord(int val) { return HiWord((uint)val); }
		public static ushort LoWord(uint val) { return (ushort)(val & 0xffff); }
		public static ushort HiWord(uint val) { return (ushort)((val >> 16) & 0xffff); }

		public static ushort LoWord(IntPtr ptr) { return LoWord((uint)ptr); }
		public static ushort HiWord(IntPtr ptr) { return HiWord((uint)ptr); }
		public static ushort LoWord(UIntPtr ptr) { return LoWord((uint)ptr); }
		public static ushort HiWord(UIntPtr ptr) { return HiWord((uint)ptr); }

		public static byte LoByte(short val) { return LoByte((ushort)val); }
		public static byte HiByte(short val) { return HiByte((ushort)val); }
		public static byte LoByte(ushort val) { return (byte)(val & 0xff); }
		public static byte HiByte(ushort val) { return (byte)((val >> 8) & 0xff); }
		#endregion

		#region // 2進数かつ先頭ゼロ埋めで文字列化。
		// ulong, uint, ushort, sbyte は基数を受け取るオーバーロードがない。

		public static string ConvertToBinaryDigitsString(byte val, int totalWidth)
		{
			return Convert.ToString(val, 2).PadLeft(totalWidth, '0');
		}

		public static string ConvertToBinaryDigitsString(short val, int totalWidth)
		{
			return Convert.ToString(val, 2).PadLeft(totalWidth, '0');
		}

		public static string ConvertToBinaryDigitsString(int val, int totalWidth)
		{
			return Convert.ToString(val, 2).PadLeft(totalWidth, '0');
		}

		public static string ConvertToBinaryDigitsString(long val, int totalWidth)
		{
			return Convert.ToString(val, 2).PadLeft(totalWidth, '0');
		}

		public static string ConvertToBinaryDigitsString(sbyte val, int totalWidth)
		{
			return ConvertToBinaryDigitsString((byte)val, totalWidth);
		}

		public static string ConvertToBinaryDigitsString(ushort val, int totalWidth)
		{
			return ConvertToBinaryDigitsString((short)val, totalWidth);
		}

		public static string ConvertToBinaryDigitsString(uint val, int totalWidth)
		{
			return ConvertToBinaryDigitsString((int)val, totalWidth);
		}

		public static string ConvertToBinaryDigitsString(ulong val, int totalWidth)
		{
			return ConvertToBinaryDigitsString((long)val, totalWidth);
		}
		#endregion
	}

	public static class MyThreadHelper
	{
		/// <summary>
		/// 現在メッセージ待ちキューの中にある全ての UI メッセージを処理します。
		/// </summary>
		public static void DoEvents()
		{
			var frame = new System.Windows.Threading.DispatcherFrame();
			var callback = new System.Windows.Threading.DispatcherOperationCallback(obj => { ((System.Windows.Threading.DispatcherFrame)obj).Continue = false; return null; });
			System.Windows.Threading.Dispatcher.CurrentDispatcher.BeginInvoke(System.Windows.Threading.DispatcherPriority.Background, callback, frame);
			System.Windows.Threading.Dispatcher.PushFrame(frame);
		}

		private static Action EmptyMethod = () => { };

		/// <summary>
		/// 強制的に再描画を実行する。拡張メソッドにはしない。
		/// </summary>
		/// <param name="uiElement"></param>
		public static void Refresh(System.Windows.UIElement uiElement)
		{
			uiElement.Dispatcher.Invoke(System.Windows.Threading.DispatcherPriority.Render, EmptyMethod);
		}

		// 明示的に作成した System.Threading.Thread すなわち
		// サブスレッドで DispatcherObject を生成するとメモリ リークする、とのことだが、
		// UIElement などだけでなく、WriteableBitmap を作成して Freeze() するだけの場合でも同様らしい。
		// （WriteableBitmap も DispatcherObject 派生）
		// http://grabacr.net/archives/1851
		// ただし、BackgroundWorker や、async/await によるスレッドプールを利用すれば、少なくともメモリ リークは発生しない模様。
		// BitmapDecoder.Create() をサブスレッドで呼び出すとリークする、という現象も同様の原理。
		// （BitmapDecoder も DispatcherObject 派生）
		// http://sssoftware.main.jp/csharp/tips/bitmap_decoder_resource_leak.html
		// VC++ で CRT ライブラリを利用する場合に Win32 API の CreateThread() 関数の使用が推奨されないのと似たようなもの？
		// RegisterClassEx() の挙動が気になるので、
		// BackgroundWorker や async/await を使う場合でも、念のため Dispatcher は明示的にシャットダウンしたほうがよさげではある。
		// http://b.starwing.net/?p=142

		public static void InvokeShutdownCurrentThreadDispatcher()
		{
			var dsp = System.Windows.Threading.Dispatcher.FromThread(System.Threading.Thread.CurrentThread);
			if (dsp != null)
			{
				dsp.InvokeShutdown();
			}
		}

		public static void ShutdownCurrentDispatcher()
		{
			System.Windows.Threading.Dispatcher.CurrentDispatcher.BeginInvokeShutdown(System.Windows.Threading.DispatcherPriority.SystemIdle);
			System.Windows.Threading.Dispatcher.Run();
		}
	}

	public static class MyInputHelper
	{
		public static bool GetCurrentKeyboardFocusedControlExists()
		{
			return (System.Windows.Input.Keyboard.FocusedElement as System.Windows.Controls.Control) != null;
		}
	}

	public static class MyVisualHelper
	{
		/// <summary>
		/// 指定した Visual 要素を、ビットマップにレンダリングして返す。
		/// </summary>
		/// <param name="outputImageWidth">出力画像の幅 [pixels]。ゼロ以下を指定すると、Visual 要素の ActualWidth から自動取得する。</param>
		/// <param name="outputImageHeight">出力画像の高さ [pixels]。ゼロ以下を指定すると、Visual 要素の ActualHeight から自動取得する。</param>
		/// <param name="dpiX">水平方向の解像度 DPI。ゼロ以下を指定すると、ディスプレイ設定から自動取得する。</param>
		/// <param name="dpiY">垂直方向の解像度 DPI。ゼロ以下を指定すると、ディスプレイ設定から自動取得する。</param>
		/// <param name="visualToRender"></param>
		/// <param name="undoTransformation">Visual 要素のアフィン変換を解除するか否か。</param>
		/// <returns></returns>
		public static System.Windows.Media.Imaging.BitmapSource CreateBitmapFromVisual(double outputImageWidth, double outputImageHeight, double dpiX, double dpiY, System.Windows.Media.Visual visualToRender, bool undoTransformation)
		{
			// cf.
			// http://social.msdn.microsoft.com/Forums/ja-JP/wpffaqja/thread/df0c59a1-f7c0-4591-9285-eeabc252a608

			if (visualToRender == null)
			{
				return null;
			}

			if (outputImageWidth <= 0)
			{
				outputImageWidth = (double)visualToRender.GetValue(System.Windows.FrameworkElement.ActualWidthProperty);
			}
			if (outputImageHeight <= 0)
			{
				outputImageHeight = (double)visualToRender.GetValue(System.Windows.FrameworkElement.ActualHeightProperty);
			}

			if (outputImageWidth <= 0 || outputImageHeight <= 0)
			{
				return null;
			}

			// PixelsPerInch() ヘルパー メソッドを利用して、画面の DPI 設定を知ることができます。
			// 指定された解像度のビットマップを作成する必要がある場合は、
			// 指定の dpiX 値と dpiY 値を RenderTargetBitmap コンストラクタに直接送ってください。
			double displayDpiX = MyDeviceHelper.GetDisplayPixelsPerInchX();
			double displayDpiY = MyDeviceHelper.GetDisplayPixelsPerInchY();
			int roundedImgWidth = (int)Math.Ceiling(outputImageWidth);
			int roundedImgHeight = (int)Math.Ceiling(outputImageHeight);
			var bmp = new System.Windows.Media.Imaging.RenderTargetBitmap(
				(dpiX > 0) ? (int)Math.Ceiling(dpiX / displayDpiX * outputImageWidth) : roundedImgWidth,
				(dpiY > 0) ? (int)Math.Ceiling(dpiY / displayDpiY * outputImageHeight) : roundedImgHeight,
				(dpiX > 0) ? dpiX : displayDpiX,
				(dpiY > 0) ? dpiY : displayDpiY,
				System.Windows.Media.PixelFormats.Pbgra32 // 必須。PixelFormats.Bgra32 などは使えない。
				);

			// 変換を解除するには、VisualBrush という方法を使用することができます。
			if (undoTransformation)
			{
				var dv = new System.Windows.Media.DrawingVisual();
				using (var dc = dv.RenderOpen())
				{
					var vb = new System.Windows.Media.VisualBrush(visualToRender);
					dc.DrawRectangle(vb, null, new System.Windows.Rect(new System.Windows.Point(), new System.Windows.Size(outputImageWidth, outputImageHeight)));
				}
				bmp.Render(dv);
			}
			else
			{
				bmp.Render(visualToRender);
			}

			return bmp;
		}

		public static System.Windows.Vector GetDpiScaleFactor(System.Windows.Media.Visual visual)
		{
			// http://grabacr.net/archives/1105
			var source = System.Windows.PresentationSource.FromVisual(visual);
			if (source != null && source.CompositionTarget != null)
			{
				return new System.Windows.Vector(
					source.CompositionTarget.TransformToDevice.M11,
					source.CompositionTarget.TransformToDevice.M22);
			}

			return new System.Windows.Vector(1.0, 1.0);
		}
	}

#if false
	public interface IWin32ModalDialogImpl
	{
		/// <summary>
		/// Win32 / MFC 相互運用のためのモーダル ダイアログ表示用ラッパー メソッド。
		/// </summary>
		/// <param name="ownerHwnd">オーナー ウィンドウのハンドル（HWND）。</param>
		/// <returns> Window.ShowDialog() の戻り値。</returns>
		bool? ShowModalDialog(IntPtr ownerHwnd);
	}
#endif

#if false
	/// <summary>
	/// 動的なカルチャ変更を実装するオブジェクトを表すインターフェイス。
	/// </summary>
	public interface IDynamicCultureChangeable
	{
		/// <summary>
		/// カルチャを変更する。
		/// </summary>
		/// <param name="newCulture">新しいカルチャ。</param>
		void ChangeCulture(System.Globalization.CultureInfo newCulture);
	}
#endif

	/// <summary>
	/// Win32 との相互運用を提供する静的ヘルパークラス。
	/// Win32 API の P/Invoke や GDI/GDI+ 連携の WIC ヘルパーなどをラップする。
	/// </summary>
	public static class MyWin32InteropHelper
	{
		public static uint GetNativeWindowThreadProcessId(IntPtr hWnd, out uint processId)
		{
			return User32DllMethodsInvoker.GetWindowThreadProcessId(hWnd, out processId);
		}

		public static uint GetNativeWindowThreadId(IntPtr hWnd)
		{
			return User32DllMethodsInvoker.GetWindowThreadProcessId(hWnd, IntPtr.Zero);
		}

		public static bool IsWindow(IntPtr hwnd)
		{
			return User32DllMethodsInvoker.IsWindow(hwnd);
		}

		public static bool HasWindowStyleMaximize(IntPtr hwnd)
		{
			return (User32DllMethodsInvoker.GetWindowStyle(hwnd) & User32DllMethodsInvoker.WindowStyles.WS_MAXIMIZE) != 0;
		}

		public static bool HasWindowStyleMinimize(IntPtr hwnd)
		{
			return (User32DllMethodsInvoker.GetWindowStyle(hwnd) & User32DllMethodsInvoker.WindowStyles.WS_MINIMIZE) != 0;
		}

		public static bool HasWindowStyleMaximizeBox(IntPtr hwnd)
		{
			return (User32DllMethodsInvoker.GetWindowStyle(hwnd) & User32DllMethodsInvoker.WindowStyles.WS_MAXIMIZEBOX) != 0;
		}

		public static bool HasWindowStyleMinimizeBox(IntPtr hwnd)
		{
			return (User32DllMethodsInvoker.GetWindowStyle(hwnd) & User32DllMethodsInvoker.WindowStyles.WS_MINIMIZEBOX) != 0;
		}

		public static bool HasWindowStyleSysMenu(IntPtr hwnd)
		{
			return (User32DllMethodsInvoker.GetWindowStyle(hwnd) & User32DllMethodsInvoker.WindowStyles.WS_SYSMENU) != 0;
		}

		public static bool HasWindowStyleVisible(IntPtr hwnd)
		{
			return (User32DllMethodsInvoker.GetWindowStyle(hwnd) & User32DllMethodsInvoker.WindowStyles.WS_VISIBLE) != 0;
		}

		// HACK: int ではなく ExWindowStyles 型を使ってフラグ演算する。
		// また、GetWindowLong()/SetWindowLong() より高レベルな GetWindowStyleEx()/SetWindowStyleEx() を用意しているので、そちらを利用する。

		public static bool HasWindowStyleExTransparent(IntPtr hwnd)
		{
			// 本来は 32bit/64bit 版兼用の GetWindowLongPtr() / SetWindowLongPtr() を使うべきだが、
			// ウィンドウ スタイル系フラグは現状 32bit 範囲分しか使われていないので問題ないらしい。
			// おそらく GWL_USERDATA でポインタ値をユーザーデータとして設定／取得する際に問題になるだけ。
			// また、32bit 版ではもともと GetWindowLongPtr() / SetWindowLongPtr() はそれぞれ
			// GetWindowLong() / SetWindowLong() にマクロで置換される仕組みで、
			// DLL に元の名前の関数エントリーポイントが存在しないはず。
			// どうしても P/Invoke で GetWindowLongPtr() / SetWindowLongPtr() を使いたい場合、
			// 実行時に IntPtr のサイズを見て分岐する必要がある。

			int extendedStyle = User32DllMethodsInvoker.GetWindowLong(hwnd, (int)User32DllMethodsInvoker.IndexOfGetWindowLong.GWL_EXSTYLE);
			return (extendedStyle & (int)User32DllMethodsInvoker.ExWindowStyles.WS_EX_TRANSPARENT) != 0;
		}

		public static void SetWindowStyleExTransparent(IntPtr hwnd, bool isTransparent = true)
		{
			int extendedStyle = User32DllMethodsInvoker.GetWindowLong(hwnd, (int)User32DllMethodsInvoker.IndexOfGetWindowLong.GWL_EXSTYLE);
			if (isTransparent)
			{
				// Change the extended window style to include WS_EX_TRANSPARENT
				User32DllMethodsInvoker.SetWindowLong(hwnd, (int)User32DllMethodsInvoker.IndexOfGetWindowLong.GWL_EXSTYLE,
					extendedStyle | (int)User32DllMethodsInvoker.ExWindowStyles.WS_EX_TRANSPARENT);
			}
			else
			{
				// Change the extended window style to exclude WS_EX_TRANSPARENT
				User32DllMethodsInvoker.SetWindowLong(hwnd, (int)User32DllMethodsInvoker.IndexOfGetWindowLong.GWL_EXSTYLE,
					extendedStyle & (~(int)User32DllMethodsInvoker.ExWindowStyles.WS_EX_TRANSPARENT));
			}
		}

		public static void SetWindowStyleExComposited(IntPtr hwnd, bool isComposited = true)
		{
			int extendedStyle = User32DllMethodsInvoker.GetWindowLong(hwnd, (int)User32DllMethodsInvoker.IndexOfGetWindowLong.GWL_EXSTYLE);
			if (isComposited)
			{
				// Change the extended window style to include WS_EX_COMPOSITED
				User32DllMethodsInvoker.SetWindowLong(hwnd, (int)User32DllMethodsInvoker.IndexOfGetWindowLong.GWL_EXSTYLE,
					extendedStyle | (int)User32DllMethodsInvoker.ExWindowStyles.WS_EX_COMPOSITED);
			}
			else
			{
				// Change the extended window style to exclude WS_EX_COMPOSITED
				User32DllMethodsInvoker.SetWindowLong(hwnd, (int)User32DllMethodsInvoker.IndexOfGetWindowLong.GWL_EXSTYLE,
					extendedStyle & (~(int)User32DllMethodsInvoker.ExWindowStyles.WS_EX_COMPOSITED));
			}
		}

		public static bool SetWindowTransparency(IntPtr hwnd, byte opacity)
		{
			// Change the extended window style to include WS_EX_LAYERED
			int extendedStyle = User32DllMethodsInvoker.GetWindowLong(hwnd, (int)User32DllMethodsInvoker.IndexOfGetWindowLong.GWL_EXSTYLE);
			User32DllMethodsInvoker.SetWindowLong(hwnd, (int)User32DllMethodsInvoker.IndexOfGetWindowLong.GWL_EXSTYLE,
				extendedStyle | (int)User32DllMethodsInvoker.ExWindowStyles.WS_EX_LAYERED);
			return User32DllMethodsInvoker.SetLayeredWindowAttributes(hwnd, 0, opacity, User32DllMethodsInvoker.LayeredWindowAttributes.LWA_ALPHA);
		}


		public static void DisableMaximizeButton(IntPtr hwnd)
		{
			var winStyle = User32DllMethodsInvoker.GetWindowStyle(hwnd);
			User32DllMethodsInvoker.SetWindowStyle(hwnd,
				winStyle & (~User32DllMethodsInvoker.WindowStyles.WS_MAXIMIZEBOX));
		}

		public static void DisableMinimizeButton(IntPtr hwnd)
		{
			var winStyle = User32DllMethodsInvoker.GetWindowStyle(hwnd);
			User32DllMethodsInvoker.SetWindowStyle(hwnd,
				winStyle & (~User32DllMethodsInvoker.WindowStyles.WS_MINIMIZEBOX));
		}

		public static void DisableWindowResizing(IntPtr hwnd)
		{
			var winStyle = User32DllMethodsInvoker.GetWindowStyle(hwnd);
			User32DllMethodsInvoker.SetWindowStyle(hwnd,
				winStyle & (~User32DllMethodsInvoker.WindowStyles.WS_THICKFRAME));
		}

		public static void DisableWindowSystemMenu(IntPtr hwnd)
		{
			var winStyle = User32DllMethodsInvoker.GetWindowStyle(hwnd);
			User32DllMethodsInvoker.SetWindowStyle(hwnd,
				winStyle & (~User32DllMethodsInvoker.WindowStyles.WS_SYSMENU));
		}

		public static void AppendWindowStyleCaption(IntPtr hwnd)
		{
			var winStyle = User32DllMethodsInvoker.GetWindowStyle(hwnd);
			User32DllMethodsInvoker.SetWindowStyle(hwnd,
				winStyle | User32DllMethodsInvoker.WindowStyles.WS_CAPTION);
		}

		static internal void AppendWindowStyleExTransparent(IntPtr hwnd)
		{
			var extendedStyle = User32DllMethodsInvoker.GetWindowStyleEx(hwnd);
			User32DllMethodsInvoker.SetWindowStyleEx(hwnd,
				extendedStyle | User32DllMethodsInvoker.ExWindowStyles.WS_EX_TRANSPARENT);
		}

		public static void RemoveSystemMenuLast2Items(IntPtr hwnd)
		{
			IntPtr hMenu = User32DllMethodsInvoker.GetSystemMenu(hwnd, false);
			if (hMenu != IntPtr.Zero)
			{
				int num = User32DllMethodsInvoker.GetMenuItemCount(hMenu);
				if (num >= 2)
				{
					// 末尾に Close コマンドがあるという前提。
					// MSDN では MF_BYCOMMAND よりも MF_BYPOSITION の使用を推奨しているようだが……
					var removeFlags = User32DllMethodsInvoker.MenuFlags.MF_BYPOSITION | User32DllMethodsInvoker.MenuFlags.MF_REMOVE;
					User32DllMethodsInvoker.RemoveMenu(hMenu, (uint)(num - 1), removeFlags); // Remove 'Close'
					User32DllMethodsInvoker.RemoveMenu(hMenu, (uint)(num - 2), removeFlags); // Remove a separator

					//User32DllMethodsInvoker.DrawMenuBar(hwnd);
				}
			}
		}

		public static string GetSystemMenuItemLabelStringClose(IntPtr hwnd)
		{
			IntPtr hMenu = User32DllMethodsInvoker.GetSystemMenu(hwnd, false);
			if (hMenu != IntPtr.Zero)
			{
				uint cmdId = (uint)Win32Commons.SystemCommandType.SC_CLOSE;
				// P/Invoke する際、メソッドの引数に StringBuilder を使うことはできるが、構造体のメンバーに StringBuilder を使うことはできない。
				// http://msdn.microsoft.com/ja-jp/library/e765dyyy%28v=vs.100%29.aspx
				var itemInfo = new User32DllMethodsInvoker.MENUITEMINFO();
				itemInfo.cbSize = (uint)System.Runtime.InteropServices.Marshal.SizeOf(itemInfo);
				itemInfo.fMask = User32DllMethodsInvoker.MenuItemInfoMaskType.MIIM_STRING;
				if (User32DllMethodsInvoker.GetMenuItemInfo(hMenu, cmdId, false, ref itemInfo))
				{
					// ウィンドウ右上のコマンド ボタンのツールヒント テキストとは違い、
					// "閉じる(&C)	Alt+F" のようにすべてのテキスト、つまりアクセス キーとショートカット キーの説明も含まれる。
					itemInfo.dwTypeData = new String('0', (int)(itemInfo.cch + 1));
					if (User32DllMethodsInvoker.GetMenuItemInfo(hMenu, cmdId, false, ref itemInfo))
					{
						return itemInfo.dwTypeData;
					}
				}
			}
			return null;
		}


		internal static bool PostLegacyWin32DialogBoxCommand(IntPtr hwnd, Win32Commons.DialogBoxCommandIds id)
		{
			return User32DllMethodsInvoker.PostMessage(hwnd, Win32Commons.Win32Message.WM_COMMAND, new IntPtr((int)id), IntPtr.Zero);
		}

		public static bool PostLegacyWin32DialogBoxCommandOK(IntPtr hwnd)
		{
			return PostLegacyWin32DialogBoxCommand(hwnd, Win32Commons.DialogBoxCommandIds.IDOK);
		}

		public static bool PostLegacyWin32DialogBoxCommandCancel(IntPtr hwnd)
		{
			return PostLegacyWin32DialogBoxCommand(hwnd, Win32Commons.DialogBoxCommandIds.IDCANCEL);
		}

		public static bool PostLegacyWin32DialogBoxCommandYes(IntPtr hwnd)
		{
			return PostLegacyWin32DialogBoxCommand(hwnd, Win32Commons.DialogBoxCommandIds.IDYES);
		}

		public static bool PostLegacyWin32DialogBoxCommandNo(IntPtr hwnd)
		{
			return PostLegacyWin32DialogBoxCommand(hwnd, Win32Commons.DialogBoxCommandIds.IDNO);
		}


		internal static bool HasLegacyWin32DialogBoxControl(IntPtr hwnd, Win32Commons.DialogBoxCommandIds id)
		{
			return User32DllMethodsInvoker.GetDlgItem(hwnd, (int)id) != IntPtr.Zero;
		}

		public static bool HasLegacyWin32DialogBoxControlOK(IntPtr hwnd)
		{
			return HasLegacyWin32DialogBoxControl(hwnd, Win32Commons.DialogBoxCommandIds.IDOK);
		}

		public static bool HasLegacyWin32DialogBoxControlCancel(IntPtr hwnd)
		{
			return HasLegacyWin32DialogBoxControl(hwnd, Win32Commons.DialogBoxCommandIds.IDCANCEL);
		}

		public static bool HasLegacyWin32DialogBoxControlYes(IntPtr hwnd)
		{
			return HasLegacyWin32DialogBoxControl(hwnd, Win32Commons.DialogBoxCommandIds.IDYES);
		}

		public static bool HasLegacyWin32DialogBoxControlNo(IntPtr hwnd)
		{
			return HasLegacyWin32DialogBoxControl(hwnd, Win32Commons.DialogBoxCommandIds.IDNO);
		}

		// Windows Forms の DialogResult には Abort, Retry, Ignore に対応する値が存在するが、
		// WPF の MessageBoxResult にはない。
		// なお、[OK] だけのときは、見た目は [OK] だが 内部コントロール／コマンド ID は IDCANCEL らしい。
		// Esc キーで閉じることができるのがその証拠。

		public static string GetLegacyWin32MessageBoxMainText(IntPtr hwnd)
		{
			var hStatic = User32DllMethodsInvoker.GetDlgItem(hwnd, (int)Win32Commons.DialogBoxCommandIds.MainStaticTextLabel);
			if (hStatic != IntPtr.Zero)
			{
				return GetWindowText(hStatic);
			}
			return null;
		}


		#region Win32/Win64 Compatible
		public static IntPtr GetWindowLongPtr(IntPtr hWnd, Int32 index)
		{
			if (IntPtr.Size == sizeof(Int32))
			{
				// Win32
				return new IntPtr(User32DllMethodsInvoker.GetWindowLong(hWnd, index));
			}
			else
			{
				// Win64
				return User32DllMethodsInvoker.GetWindowLongPtr(hWnd, index);
			}
		}

		public static IntPtr SetWindowLongPtr(IntPtr hWnd, Int32 index, IntPtr newStyle)
		{
			if (IntPtr.Size == sizeof(Int32))
			{
				// Win32
				return new IntPtr(User32DllMethodsInvoker.SetWindowLong(hWnd, index, newStyle.ToInt32()));
			}
			else
			{
				// Win64
				return User32DllMethodsInvoker.SetWindowLongPtr(hWnd, index, newStyle);
			}
		}

		public static IntPtr GetClassLongPtr(IntPtr hWnd, Int32 index)
		{
			if (IntPtr.Size == sizeof(Int32))
			{
				// Win32
				return new IntPtr((int)User32DllMethodsInvoker.GetClassLong(hWnd, index));
			}
			else
			{
				// Win64
				return new IntPtr((long)User32DllMethodsInvoker.GetClassLongPtr(hWnd, index).ToUInt64());
			}
		}

		public static IntPtr SetClassLongPtr(IntPtr hWnd, Int32 index, IntPtr newLong)
		{
			if (IntPtr.Size == sizeof(Int32))
			{
				// Win32
				return new IntPtr((int)User32DllMethodsInvoker.SetClassLong(hWnd, index, newLong.ToInt32()));
			}
			else
			{
				// Win64
				return new IntPtr((long)User32DllMethodsInvoker.SetClassLongPtr(hWnd, index, newLong).ToUInt64());
			}
		}

		public static IntPtr GetModuleHandleFromWindow(IntPtr hWnd)
		{
			return GetClassLongPtr(hWnd, (int)User32DllMethodsInvoker.IndexOfGetClassLong.GCL_HMODULE);
		}

		public static IntPtr GetInstanceHandleFromWindow(IntPtr hWnd)
		{
			return GetWindowLongPtr(hWnd, (int)User32DllMethodsInvoker.IndexOfGetWindowLong.GWL_HINSTANCE);
		}
		#endregion

#if false
		// C/C++ の memcpy() 相当を実装する際、kernel32.dll の CopyMemory() Win32 API に依存したくなければこちら。
		// Xbox 360 / Xbox One や Windows ストア アプリではこちらを使うことになると思われる。
		// どのみちポインタを使う必要があるので、素直に for ループを回して1バイトずつコピーしてもよい気がするが……
		// CopyTo() メソッドはそのあたり最適化されている？
		internal static unsafe void CopyMemory(IntPtr dst, IntPtr src, int size)
		{
			using (var streamSrc = new System.IO.UnmanagedMemoryStream((byte*)src, size))
			{
				using (var streamDst = new System.IO.UnmanagedMemoryStream((byte*)dst, size))
				{
					streamSrc.CopyTo(streamDst);
				}
			}
		}
#endif

		/// <summary>
		/// 2つの矩形の共通部分（積）を返す。共通部分がない場合は System.Windows.Int32Rect.Empty を返す。
		/// </summary>
		/// <param name="r1"></param>
		/// <param name="r2"></param>
		/// <returns></returns>
		public static System.Windows.Int32Rect CreateRectIntersect(System.Windows.Int32Rect r1, System.Windows.Int32Rect r2)
		{
			// System.Drawing.Rectangle.Intersect()、
			// System.Drawing.RectangleF.Intersect()、
			// System.Windows.Rect.Intersect() および
			// Windows.Foundation.Rect.Intersect() 同様の実装になっているはず。
			// 面倒なので拡張メソッド実装にはしない。
			// GDI+、WPF、WinRT ストア アプリで共通して使えるデータ型（＋豊富なユーティリティ メソッド）が標準定義されていると楽なのだが……
			int left = Math.Max(r1.X, r2.X);
			int top = Math.Max(r1.Y, r2.Y);
			int right = Math.Min(r1.X + r1.Width, r2.X + r2.Width);
			int bottom = Math.Min(r1.Y + r1.Height, r2.Y + r2.Height);
			int width = right - left;
			int height = bottom - top;
			if (width <= 0 || height <= 0)
			{
				return System.Windows.Int32Rect.Empty;
			}
			return new System.Windows.Int32Rect(left, top, width, height);
		}

		/// <summary>
		/// 2つの矩形の境界矩形（和）を返す。
		/// </summary>
		/// <param name="r1"></param>
		/// <param name="r2"></param>
		/// <returns></returns>
		public static System.Windows.Int32Rect CreateRectUnion(System.Windows.Int32Rect r1, System.Windows.Int32Rect r2)
		{
			// System.Drawing.Rectangle.Union()、
			// System.Drawing.RectangleF.Union()、
			// System.Windows.Rect.Union() および
			// Windows.Foundation.Rect.Union() 同様の実装になっているはず。
			int left = Math.Min(r1.X, r2.X);
			int top = Math.Min(r1.Y, r2.Y);
			int right = Math.Max(r1.X + r1.Width, r2.X + r2.Width);
			int bottom = Math.Max(r1.Y + r1.Height, r2.Y + r2.Height);
			int width = right - left;
			int height = bottom - top;
			System.Diagnostics.Debug.Assert(width >= 0 && height >= 0);
			return new System.Windows.Int32Rect(left, top, width, height);
		}


		public static System.Windows.Int32Rect CalcClientIntersectRect(IntPtr hwnd, System.Windows.Int32Rect clippingRect)
		{
			// デスクトップに対するウィンドウのスクリーン座標を取得しておく。
			var winRect = GetWindowRect(hwnd);
			// クライアント領域のサイズを取得する。
			var clientRect = GetClientRect(hwnd); // (left, top) は常に (0, 0) が返る。
			var clientOriginPos = new Win32Commons.POINT();
			// ウィンドウのクライアント左上をスクリーン座標に直す。
			User32DllMethodsInvoker.ClientToScreen(hwnd, ref clientOriginPos);
			// ウィンドウから見たクライアント領域の相対矩形を計算。
			var targetRect = new System.Windows.Int32Rect(
				clientOriginPos.x - winRect.X,
				clientOriginPos.y - winRect.Y,
				clientRect.Width, clientRect.Height);
			clippingRect.X += targetRect.X;
			clippingRect.Y += targetRect.Y;
			// ユーザー定義クリッピング矩形も考慮する。
			var intersectRect = CreateRectIntersect(targetRect, clippingRect);
			if (!intersectRect.HasArea)
			{
				// 交差しない場合、元のクライアント領域の矩形を使う。
				intersectRect = targetRect;
			}
			return intersectRect;
		}

		public static IntPtr GetOwnerWindow(IntPtr hwnd)
		{
			return User32DllMethodsInvoker.GetWindow(hwnd, User32DllMethodsInvoker.CommandOfGetWindow.GW_OWNER);
		}

		public static IntPtr GetNextWindow(IntPtr hwnd)
		{
			return User32DllMethodsInvoker.GetWindow(hwnd, User32DllMethodsInvoker.CommandOfGetWindow.GW_HWNDNEXT);
		}

		public static IntPtr GetPrevWindow(IntPtr hwnd)
		{
			return User32DllMethodsInvoker.GetWindow(hwnd, User32DllMethodsInvoker.CommandOfGetWindow.GW_HWNDPREV);
		}

		public static bool HasOwnerWindow(IntPtr hwnd)
		{
			return GetOwnerWindow(hwnd) != IntPtr.Zero;
		}

		public static bool HasNoOwnerWindow(IntPtr hwnd)
		{
			return GetOwnerWindow(hwnd) == IntPtr.Zero;
		}

		public static string GetWindowText(IntPtr hwnd)
		{
			if (!User32DllMethodsInvoker.IsWindow(hwnd))
			{
				return null;
			}
			int winTextLength = User32DllMethodsInvoker.GetWindowTextLength(hwnd);
			if (winTextLength > 0)
			{
				var sb = new StringBuilder(winTextLength + 1); // 終端 null 文字の分。
				User32DllMethodsInvoker.GetWindowText(hwnd, sb, sb.Capacity);
				return sb.ToString();
			}
			else
			{
				return String.Empty;
			}
		}

		public static IntPtr GetDesktopWindow()
		{
			return User32DllMethodsInvoker.GetDesktopWindow();
		}

		public static bool FlashWindowEx(IntPtr hwnd, User32DllMethodsInvoker.FlashWindowMode mode, uint count, uint timeoutMilliseconds = 0)
		{
			var fwi = new User32DllMethodsInvoker.FLASHWINFO() { dwFlags = mode, uCount = count, dwTimeout = timeoutMilliseconds };
			fwi.cbSize = (uint)System.Runtime.InteropServices.Marshal.SizeOf(fwi);
			fwi.hwnd = hwnd;
			return User32DllMethodsInvoker.FlashWindowEx(ref fwi);
		}

		static System.Windows.Int32Rect ToInt32Rect(this Win32Commons.RECT rect)
		{
			return new System.Windows.Int32Rect(rect.left, rect.top, rect.Width, rect.Height);
		}

		public static System.Windows.Int32Rect GetWindowRect(IntPtr hwnd)
		{
			var tempRect = new Win32Commons.RECT();
			User32DllMethodsInvoker.GetWindowRect(hwnd, ref tempRect);
			return tempRect.ToInt32Rect();
		}

		public static System.Windows.Int32Rect GetClientRect(IntPtr hwnd)
		{
			var tempRect = new Win32Commons.RECT();
			User32DllMethodsInvoker.GetClientRect(hwnd, ref tempRect);
			return tempRect.ToInt32Rect();
		}

		public static System.Windows.Int32Rect GetWorkAreaRect()
		{
			var tempRect = new Win32Commons.RECT();
			User32DllMethodsInvoker.SystemParametersInfo(User32DllMethodsInvoker.SystemParametersInfoType.SPI_GETWORKAREA, 0, ref tempRect, 0);
			return new System.Windows.Int32Rect(tempRect.left, tempRect.top, tempRect.Width, tempRect.Height);
		}

		public static List<IntPtr> EnumVisibleWindows()
		{
#if true
			// デスクトップの矩形はプライマリ モニターの矩形？
			System.Diagnostics.Debug.WriteLine("Desktop Info:");
			IntPtr hDesktopWnd = User32DllMethodsInvoker.GetDesktopWindow();
			// HWND 型自体はポインタ型で、64bit ネイティブ プログラムでは 8 バイトだが、Win64 では実質下位 4 バイト分しか使われないらしい
			// （でないと Win32 アプリと x64 アプリとで HWND の値が異なる結果になってしまい、HWND 経由でプロセス間通信できない）。
			System.Diagnostics.Debug.WriteLine("hWnd = 0x{0}, Title = \"{1}\"", hDesktopWnd.ToString("X8"), GetWindowText(hDesktopWnd));
			var desktopRect = new Win32Commons.RECT();
			User32DllMethodsInvoker.GetWindowRect(hDesktopWnd, ref desktopRect);
			System.Diagnostics.Debug.WriteLine("DesktopRect = ({0}, {1}, {2}, {3})",
				desktopRect.left, desktopRect.top, desktopRect.right, desktopRect.bottom);
#endif

			var winHandleList = new List<IntPtr>();
			// タスク マネージャーの [アプリケーション] タブに表示されるようなトップレベル ウィンドウを列挙する。
			// 自分自身も一応列挙に含める。呼び出し側でフィルタリングすればよい。
			// "スタート" を列挙したくない場合、「オーナーウィンドウを持たない」という条件でフィルタリングできる。
			// "Program Manager"（≒デスクトップ）も列挙されるのを何とかできないか？
			// GetWindowText() の結果を文字列比較するだけでは不十分。ちなみに GetDesktopWindow() で得られるデスクトップ ハンドルとは異なるウィンドウ。
			User32DllMethodsInvoker.EnumWindows(new User32DllMethodsInvoker.EnumWindowsProcDelegate((hWnd, lParam) =>
				{
					if (User32DllMethodsInvoker.IsWindowVisible(hWnd) && HasNoOwnerWindow(hWnd))
					{
						string title = GetWindowText(hWnd);
						if (title != String.Empty)
						{
#if false
							// {0:X8} ではゼロ パディングにならない。
							System.Diagnostics.Debug.WriteLine("hWnd = 0x{0}, Title = \"{1}\"", hWnd.ToString("X8"), title);

							var winPlacement = new User32DllMethodsInvoker.WINDOWPLACEMENT();
							winPlacement.InitializeLength();
							User32DllMethodsInvoker.GetWindowPlacement(hWnd, ref winPlacement);
							System.Diagnostics.Debug.WriteLine("({0}, {1}), ({2}, {3})",
								winPlacement.ptMinPosition.x,
								winPlacement.ptMinPosition.y,
								winPlacement.ptMaxPosition.x,
								winPlacement.ptMaxPosition.y);
#endif
							winHandleList.Add(hWnd);
						}
					}
					return true; // 列挙を続行。
				}),
				IntPtr.Zero);
			return winHandleList;
		}

		/// <summary>
		/// 指定されたプロセスが現在表示しているモーダル ダイアログのウィンドウ ハンドルをすべて列挙する。
		/// </summary>
		/// <param name="ownerProcessId"></param>
		/// <returns></returns>
		public static List<IntPtr> EnumSubModalDialogs(uint ownerProcessId)
		{
			var winHandleList = new List<IntPtr>();
			User32DllMethodsInvoker.EnumWindows(new User32DllMethodsInvoker.EnumWindowsProcDelegate((hWnd, lParam) =>
				{
					uint procId;
					User32DllMethodsInvoker.GetWindowThreadProcessId(hWnd, out procId);
					if (procId == ownerProcessId)
					{
						var winStyle = User32DllMethodsInvoker.GetWindowStyle(hWnd);
						if (winStyle.HasFlag(User32DllMethodsInvoker.WindowStyles.DS_MODALFRAME))
						{
							winHandleList.Add(hWnd);
						}
					}
					return true; // 列挙を続行。
				}),
				IntPtr.Zero);
			return winHandleList;
		}

		// System.Diagnostics.Process.MainWindowHandle は状況によって動的に切り替わってしまう。
		// したがってこのプロパティでプロセスのメインウィンドウ（WPF の System.Windows.Application.MainWindow など）を安定して取得することはできない。
		// Windows Forms の場合、MainWindowHandle に影響するのはメニュー、コンテキストメニュー、ツールヒント、そしてウィンドウのシステムメニュー。他にもあるかもしれない。
		// ツールヒント（ツールチップ）も一種のウィンドウ。
		// CommCtrl.h で定義されている TOOLTIPS_CLASS すなわち "tooltips_class32" とウィンドウクラス名を比較して、ツールヒントか否かを判定する？
		// WPF の Menu/ContextMenu/ToolTip は MainWindowHandle に影響しないようだが、ウィンドウのシステムメニューは影響する。

		/// <summary>
		/// プロセスに属するトップレベルの（オーナーを持たない）可視ウィンドウをすべて列挙する。
		/// </summary>
		/// <param name="ownerProcessId"></param>
		/// <returns></returns>
		public static List<IntPtr> EnumProcessRootVisibleWindows(uint ownerProcessId)
		{
			var winHandleList = new List<IntPtr>();
			User32DllMethodsInvoker.EnumWindows(new User32DllMethodsInvoker.EnumWindowsProcDelegate((hWnd, lParam) =>
				{
					uint procId;
					User32DllMethodsInvoker.GetWindowThreadProcessId(hWnd, out procId);
					if (procId == ownerProcessId)
					{
						if (User32DllMethodsInvoker.IsWindowVisible(hWnd) && HasNoOwnerWindow(hWnd))
						{
							winHandleList.Add(hWnd);
						}
					}
					return true; // 列挙を続行。
				}),
				IntPtr.Zero);
			return winHandleList;
		}


		public static void WakeupProcesses(string procName)
		{
			var targetProcesses = System.Diagnostics.Process.GetProcessesByName(procName);
			if (targetProcesses != null)
			{
				foreach (var proc in targetProcesses)
				{
					WakeupWindow(proc.MainWindowHandle);
				}
			}
		}

		/// <summary>
		/// 特定のウィンドウを強制的に最前面に表示する。
		/// </summary>
		/// <param name="hwnd">
		/// 通常は Process.MainWindowHandle を渡せばよいが、タスク バーに表示されていないウィンドウのハンドルは取得できないので注意。
		/// タスクバーに表示されていないトップレベルのウィンドウも取得したい場合、Win32 EnumWindows() API などを使うしかないらしい。
		/// </param>
		public static void WakeupWindow(IntPtr hwnd)
		{
			// ウィンドウが最小化されていれば元に戻す。
			if (User32DllMethodsInvoker.IsIconic(hwnd))
			{
				User32DllMethodsInvoker.ShowWindowAsync(hwnd, User32DllMethodsInvoker.CommandOfShowWindow.SW_RESTORE);
			}

			// ウィンドウを最前面に表示する。
			User32DllMethodsInvoker.SetForegroundWindow(hwnd);
		}

		public static IntPtr FindDesktopTaskBarListWindowHandle()
		{
			// クラス名は Windows 8.1 上で Spy++ を使って調べた結果。Windows 7 と同じ。
			// Win8.1 の場合、タスク バーの端には EdgeUiInputWndClass というクラス名のトップレベル ウィンドウも存在する模様。
			// おそらくマウスによるスワイプ代替操作などの目的で存在する隠しウィンドウらしい。
			// HACK: Windows 10 だとどうなる？
			var hWndTaskBarRoot = User32DllMethodsInvoker.FindWindow("Shell_TrayWnd", null); // タスク バー全体。
#if false
			return hWndTaskBarRoot;
#else
			if (hWndTaskBarRoot != IntPtr.Zero)
			{
				var retVal = IntPtr.Zero;
				User32DllMethodsInvoker.EnumChildWindows(hWndTaskBarRoot,
					(hWnd, lParam) =>
					{
						var classNameBuffer = new StringBuilder(128);
						if (User32DllMethodsInvoker.GetClassName(hWnd, classNameBuffer, classNameBuffer.Capacity) != 0)
						{
							// アプリケーションのタスク バー ボタン リスト表示領域。
							if (classNameBuffer.ToString() == "MSTaskListWClass")
							{
								retVal = hWnd;
								return false; // 列挙を停止。
							}
						}
						return true; // 列挙を続行。
					}, IntPtr.Zero);
				if (retVal == IntPtr.Zero)
				{
					System.Diagnostics.Debug.WriteLine("Failed to find the task list window!!");
				}
				return retVal;
			}
			else
			{
				System.Diagnostics.Debug.WriteLine("Failed to find the root window of task bar!!");
				return IntPtr.Zero;
			}
#endif
		}

		public static System.Diagnostics.FileVersionInfo GetProcessExeFileVersionInfo(IntPtr hProc)
		{
			// アプリが 32bit で、調査対象が 64bit プロセスの場合、Process.MainModule や Process.Modules にアクセスしようとすると Win32Exception がスローされる。
			// 32bit プロセスは 64bit プロセスのモジュールにアクセスできないという制約があるらしい。EXE ファイルのパスやバージョン番号を取得することすらままならない。
			// Windows Vista 以降で実装された Win32 API である QueryFullProcessImageName() と、FileVersionInfo.GetVersionInfo() を使うしかなさそう。
			// .NET 4.5 は XP を動作対象外にしているので、もはや問題ではない。

			const int strBufCapacity = 1024;
			int inoutStrLen = strBufCapacity;
			var strBuf = new StringBuilder(strBufCapacity);
			if (Kernel32DllMethodsInvoker.QueryFullProcessImageName(hProc, 0, strBuf, ref inoutStrLen))
			{
				return System.Diagnostics.FileVersionInfo.GetVersionInfo(strBuf.ToString());
			}
			else
			{
				return null;
			}
		}
	}

	public sealed class MyLogicalAscendingStringComparer : IComparer<string>
	{
		public int Compare(string a, string b)
		{
			return ShellWApiDllMethodsInvoker.StrCmpLogicalW(a ?? "", b ?? "");
		}
	}

	public sealed class MyLogicalDescendingStringComparer : IComparer<string>
	{
		public int Compare(string a, string b)
		{
			return ShellWApiDllMethodsInvoker.StrCmpLogicalW(b ?? "", a ?? "");
		}
	}

	/// <summary>
	/// カスタム プロシージャーのアタッチとデタッチを管理する RAII クラス。
	/// </summary>
	/// <remarks>
	/// IDisposable を実装するのも一つの手だが、アプリ起動から終了までずっと生存し続ける必要がある場合は、
	/// IDisposable を実装することによるメリットはほとんどない。
	/// </remarks>
	public class MyCustomWinProcManagerBase
	{
		System.Windows.Interop.HwndSource _source;
		System.Windows.Interop.HwndSourceHook _hook;

		public void AttachCustomWndProc(IntPtr hwnd, System.Windows.Interop.HwndSourceHook customWndProc)
		{
			System.Diagnostics.Debug.Assert(this._source == null && this._hook == null);
			this._source = System.Windows.Interop.HwndSource.FromHwnd(hwnd);
			this._hook = customWndProc; // デリゲートのキャッシュ。
			System.Diagnostics.Debug.Assert(this._hook != null);
			this._source.AddHook(this._hook);
		}

		public void DetachCustomWndProc()
		{
			//System.Diagnostics.Debug.Assert(this.source != null && this.hook != null);
			if (this._source != null)
			{
				if (this._hook != null)
				{
					this._source.RemoveHook(this._hook);
				}
				this._source.Dispose();
			}
			this._source = null;
			this._hook = null;
		}
	}

	/// <summary>
	/// レイヤード ウィンドウ用のユーティリティ派生クラス。
	/// </summary>
	public class MyCustomLayeredWinProcManager : MyCustomWinProcManagerBase
	{
		public event Action PreMinimized;
		//public event Action PreMaximized;

		// HACK: 物理ピクセルではなく論理ピクセルを指定できるようにする。

		public int MinWindowWidth { get; set; }
		public int MinWindowHeight { get; set; }

		// C++ の場合、派生クラスでオーバーロードを定義すると基底クラスのメソッドすべてを隠ぺいすることになるが、C# では両方とも可視になる。
		// C# で new キーワードを使う必要があるのは、まったく同じシグネチャのメソッドを派生クラスで定義するときのみ。
		public void AttachCustomWndProc(IntPtr hwnd)
		{
			base.AttachCustomWndProc(hwnd, this.MyWndProc);
		}

		private IntPtr MyWndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled)
		{
			if (msg == (int)Win32Commons.Win32Message.WM_GETMINMAXINFO)
			{
				var result = this.OnGetMinMaxInfo(hwnd, wParam, lParam);
				if (result != null)
				{
					handled = true;
					return result.Value;
				}
			}

			if (msg == (int)Win32Commons.Win32Message.WM_WINDOWPOSCHANGING)
			{
				var param = (Win32Commons.WINDOWPOS)System.Runtime.InteropServices.Marshal.PtrToStructure(lParam, typeof(Win32Commons.WINDOWPOS));
				// 最小化するときに Windows によってこの位置に移動されるらしい？　システム設定に依らない固定値？
				// 最小化されているアプリの画面内容をキャプチャすることは不可能。
				if (param.x == -32000 && param.y == -32000)
				{
					System.Diagnostics.Debug.WriteLine("Special position for Minimized window.");
					if (this.PreMinimized != null)
					{
						this.PreMinimized();
					}
				}
#if false
				// 最大化するときに Windows によってこの位置に移動されるらしい？　システム設定に依らない固定値？
				// 実際に最大化されているアプリの画面内容を BitBlt でキャプチャしてみると、端に余白があることが分かる。
				// ただし、WM_GETMINMAXINFO で設定した値によって変化しうるらしい。つまりアプリ固有値。
				if (param.x == -7 && param.y == -7)
				{
					System.Diagnostics.Debug.WriteLine("Special position for Maximized window.");
					if (this.PreMaximized != null)
					{
						this.PreMaximized();
					}
				}
#endif
				// http://stackoverflow.com/questions/926758/window-statechanging-event-in-wpf
				// http://blogs.msdn.com/b/oldnewthing/archive/2004/10/28/249044.aspx
			}

			// process minimize button
			if (msg == (int)Win32Commons.Win32Message.WM_SYSCOMMAND)
			{
				switch ((Win32Commons.SystemCommandType)wParam.ToInt32())
				{
					case Win32Commons.SystemCommandType.SC_MINIMIZE:
						// タスク バーのクリックやシステム コマンドの [最小化] はここで事前フックできるが、
						// Aero Shake / Aero Preview による最小化はフックできない。
						// WPF の Window.WindowState を操作した場合も同様。
						System.Diagnostics.Debug.WriteLine("SystemCommand.Minimize executed.");
						break;
					case Win32Commons.SystemCommandType.SC_MAXIMIZE:
						// システム コマンドの [最大化] はここで事前フックできるが、
						// タイトル バーのダブルクリックや Aero Snap による最大化はフックできない。
						// WPF の Window.WindowState を操作した場合も同様。
						System.Diagnostics.Debug.WriteLine("SystemCommand.Maximize executed.");
						break;
					case Win32Commons.SystemCommandType.SC_RESTORE:
						// タスク バーのクリックやシステム コマンドの [元のサイズに戻す] はここで事前フックできるが、
						// タイトル バーのダブルクリックや Aero Snap / Aero Preview による復元はフックできない。
						// WPF の Window.WindowState を操作した場合も同様。
						System.Diagnostics.Debug.WriteLine("SystemCommand.Restore executed.");
						break;
					default:
						break;
				}
			}

			handled = false;
			return IntPtr.Zero;
		}

		private IntPtr? OnGetMinMaxInfo(IntPtr hwnd, IntPtr wParam, IntPtr lParam)
		{
			// WPF で WindowStyle="None" を指定したウィンドウを最大化した際にタスク バー領域を覆ってしまうのを防止する。
			// アプリ起動中にタスク バーの表示位置が変更された場合にも対応できる。
			// http://karamemo.hateblo.jp/entry/20130527/1369658222
			// ディスプレイ解像度も同様にして、システム パラメーターの変更イベント（Win32 メッセージ）をフックすることで対応できるか？
			var monitor = User32DllMethodsInvoker.MonitorFromWindow(hwnd, User32DllMethodsInvoker.MonitorFlagType.MONITOR_DEFAULTTONEAREST);
			if (monitor == IntPtr.Zero)
			{
				return null;
			}
			var monitorInfo = new User32DllMethodsInvoker.MONITORINFO();
			monitorInfo.InitializeSize();
			if (!User32DllMethodsInvoker.GetMonitorInfo(monitor, ref monitorInfo))
			{
				return null;
			}
			var workingRectangle = monitorInfo.rcWork;
			var monitorRectangle = monitorInfo.rcMonitor;
			var minmax = (User32DllMethodsInvoker.MINMAXINFO)System.Runtime.InteropServices.Marshal.PtrToStructure(lParam, typeof(User32DllMethodsInvoker.MINMAXINFO));
			minmax.ptMaxPosition.x = Math.Abs(workingRectangle.left - monitorRectangle.left);
			minmax.ptMaxPosition.y = Math.Abs(workingRectangle.top - monitorRectangle.top);
			minmax.ptMaxSize.x = workingRectangle.Width;
			minmax.ptMaxSize.y = workingRectangle.Height;
			// WPF の Window.MinWidth や Window.MinHeight にはそれぞれ限界があるが、
			// ここで MINMAXINFO.ptMinTrackSize にゼロを設定するとさらに小さいウィンドウにすることができてしまう。
			minmax.ptMinTrackSize.x = this.MinWindowWidth;
			minmax.ptMinTrackSize.y = this.MinWindowHeight;
			//minmax.ptMaxTrackSize = minmax.ptMaxSize; // いまいち用途不明。
			// マネージ コードによる変更内容をアンマネージ側に書き戻し（置き換え）。
			System.Runtime.InteropServices.Marshal.StructureToPtr(minmax, lParam, true);
			return IntPtr.Zero;
		}
	}

	// マネージ コードでも可能なグローバル フックは、WH_KEYBOARD_LL, WH_MOUSE_LL の2つのみらしい。
	// http://azumaya.s101.xrea.com/wiki/index.php?%B3%D0%BD%F1%2FC%A2%F4%2F%A5%B0%A5%ED%A1%BC%A5%D0%A5%EB%A5%D5%A5%C3%A5%AF

	public class MyCustomWindowsHookProcManagerBase
	{
		IntPtr _hook;
		User32DllMethodsInvoker.HookProcDelegate _proc;

		public void AttachCustomWndProc(User32DllMethodsInvoker.WindowsHookType hookType, User32DllMethodsInvoker.HookProcDelegate customWndProc, IntPtr hModule, uint threadId = 0)
		{
			this._proc = customWndProc; // デリゲートのキャッシュ。
			System.Diagnostics.Debug.Assert(this._proc != null);
			this._hook = User32DllMethodsInvoker.SetWindowsHookEx(hookType,
				 this._proc,
				 hModule,
				 threadId);
			System.Diagnostics.Debug.Assert(this._hook != IntPtr.Zero);
		}

		public void DetachCustomWndProc()
		{
			if (this._hook != IntPtr.Zero)
			{
				User32DllMethodsInvoker.UnhookWindowsHookEx(this._hook);
			}
			this._hook = IntPtr.Zero;
			this._proc = null;
		}

		protected IntPtr CallNextHookEx(int nCode, IntPtr wParam, IntPtr lParam)
		{
			System.Diagnostics.Debug.Assert(this._hook != null);
			return User32DllMethodsInvoker.CallNextHookEx(this._hook, nCode, wParam, lParam);
		}
	}
}
