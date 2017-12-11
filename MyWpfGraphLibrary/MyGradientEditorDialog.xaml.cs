using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace MyWpfGraphLibrary
{
	/// <summary>
	/// MyGradientEditorDialog.xaml の相互作用ロジック
	/// </summary>
	public partial class MyGradientEditorDialog : Window
	{
		#region Properties

		public MyGradientEditor.IManagedEventListener EventListener
		{
			set { this.gradientEditor.EventListener = value; }
			get { return this.gradientEditor.EventListener; }
		}
		public bool CanNotifyListener { get { return this.EventListener != null; } }

		/// <summary>
		/// <para>
		/// false にすると通常動作で完全クローズされず、インスタンスの再利用が可能になる。
		/// </para>
		/// <para>
		/// true にすると通常動作で完全クローズされて、インスタンスの再利用が不可能になる。
		/// </para>
		/// </summary>
		public bool CanEnforcedClose { get; set; }

		public ViewModels.MyGradientStopViewModel[] GetGradientColorStopPinsArray()
		{
			return this.gradientEditor.GetGradientStops().ToArray();
		}
		public void SetGradientColorStopPinsArray(ViewModels.MyGradientStopViewModel[] stopPins)
		{
			this.gradientEditor.SetGradientStops(stopPins);
		}

		/// <summary>
		/// [OK] ならば true になる。それ以外は false になる。
		/// </summary>
		public bool IsAccepted { get; protected set; }

		#endregion

		/// <summary>
		/// デフォルト コンストラクタ。
		/// </summary>
		public MyGradientEditorDialog()
		{
			InitializeComponent();

			// ただ単に最大化・最小化ボタンをなくしたい場合、ToolWindow スタイルを使うのが楽だが、
			// ToolWindow スタイルにしただけでは、最大化や最小化のシステム コマンドは残ったまま。
			// 最大化も最小化も両方無効化したい場合、Window.ResizeMode を NoResize に設定するとよい。
			// 最大化ボタン＆システム コマンドを無効にして最小化ボタン＆システム コマンドを有効にしたい場合は、
			// P/Invoke で Win32 ウィンドウ スタイルを制御する処理が必要。
			// なお、ToolWindow は通例モードレスな子ウィンドウに使う。
			// ちなみにタイトル バーがなくても、[Alt + Space] でシステム コマンド メニューを出現させることができる。

			//this.WindowStyle = System.Windows.WindowStyle.ToolWindow;
		}

		public bool ShowModalDialog()
		{
			// Windows Forms の Form.ShowDialog() とは違って、
			// WPF の Window.ShowDialog() はデフォルトでは複数回呼び出せない（クローズした後のインスタンスを再利用できない）。
			// InvalidOperationException が発生する。
			// Window.Show() も同様。
			// モードレスだろうがモーダルだろうが、インスタンス再利用のためには Closing イベントでの制御が必要。
			this.IsAccepted = false;
			this.ShowDialog();
			return this.IsAccepted;
		}

		private void Window_Loaded(object sender, RoutedEventArgs e)
		{
		}

		private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
		{
			// オーナーウィンドウが閉じられるときは、Closing イベントは発生しないらしい。
			// オーナーに対して Win32 DestroyWindow() が呼ばれるタイミングで自動的に Close() されるらしい。

			// 普段はクローズ ボタンが押されるなどして Close() が呼ばれても、非表示にするだけ。実際には Win32 ウィンドウが破棄されることはない。
			if (!this.CanEnforcedClose)
			{
				e.Cancel = true;
				this.Hide();
				// キャンセル処理を入れないと、アンマネージ リソース（Win32 ウィンドウ ハンドル）が破棄されるため、再利用は不可能となる。
				// ただし Close() しただけではマネージ リソースのほうは解放されないので、CLR プロパティには普通にアクセスできるものが多い。
				// ちなみにキャンセルすると、DialogResult に true/false を入れていたとしても null になってしまう模様。
				// モーダル ダイアログ風に使用する場合は注意。
			}
		}

		private void buttonOK_Click(object sender, RoutedEventArgs e)
		{
			this.IsAccepted = true;
			this.DialogResult = true;
			// Window.Closing をキャンセルする処理を入れると、DialogResult は強制的に null になるらしい。
		}
	}
}
