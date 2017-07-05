using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
//using System.Diagnostics;
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


// Photoshop ライクのグラデーション エディタとする。
// ちなみに下記のようなブラウザで動作する JavaScript 製の CSS3 用グラデーション エディタも存在する。
// http://www.colorzilla.com/gradient-editor/
// 
// 「WPF Property Grid」というのも存在するが、これもグラデーション ブラシを編集できるコントロールが組み込まれている模様。
// ただしこのブラシ エディタ コントロールは SharpDevelop のコンポーネントらしく、LGPL ライセンスらしい。
// WPF Property Grid 自体は Ms-PL ライセンスらしい。
// http://wpg.codeplex.com/
// 
// グラデーション オフセットの UI 設定値は 0[%]~100[%] だが、内部では 0.0～1.0 の正規化された値で管理する。
// 
// GradientBrush.GradientStops に ObservableCollection をバインディングする方法もあるが、複雑。
// https://stackoverflow.com/questions/41758309/wpf-bind-gradientstopcollection
// 
// SolidColorBrush オブジェクトに読み取り専用（定義済み）の Brushes メンバーなどが割り当てられている場合、
// SolidColorBrush.Color プロパティを直接変更できない。
// 同値を持つ SolidColorBrush オブジェクトの明示的作成が必要。

namespace MyWpfGraphLibrary
{
	/// <summary>
	/// MyGradientEditor.xaml の相互作用ロジック
	/// </summary>
	public partial class MyGradientEditor : UserControl
	{
		public interface IManagedEventListener
		{
			//void OnUpdate(MyGradientStopPin[] gradientStopPins);
			void OnUpdate(int gradientStopCount);
		}

		#region Properties

		public IManagedEventListener EventListener { set; get; }
		public bool CanNotifyListener { get { return this.EventListener != null; } }

		#endregion

		static GradientStop CreateGradientStopFromVM(ViewModels.MyGradientStopViewModel src)
		{
			// Color プロパティは R/W だが、Offset プロパティは読み取り専用なので、既存オブジェクトの動的変更はできず、再割り当てが必要。
			return new GradientStop(src.Color, src.Offset);
		}

		static double NormalClamp(double val)
		{
			return MyMiscHelpers.MyGenericsHelper.Clamp(val, 0.0, 1.0);
		}

		public IEnumerable<ViewModels.MyGradientStopViewModel> GetGradientStops()
		{
			// グラデーション Offset の昇順に並べ替えたものを返す。
			return from elem in this._viewModel.GradientStops orderby elem.Offset ascending select elem;
		}

		public void SetGradientStops(IEnumerable<ViewModels.MyGradientStopViewModel> stops)
		{
			// スワップの実装は VisualParent の関係からちょっと大変。全削除して全追加の手順をとる。
			this._viewModel.GradientStops.Clear();
			this.gradientBrush.GradientStops.Clear();
			this._viewModel.CurrentGradientStop = new ViewModels.MyGradientStopViewModel();
			this._isHoldingPin = false;
			this._isDraggingPin = false;
			foreach (var elem in stops)
			{
				// ソートする必要はない。
				double normalizedOffset = NormalClamp(elem.Offset);
				var stop = CreateGradientStopFromVM(elem);
				elem.Tag = stop;
				this._viewModel.GradientStops.Add(elem);
				this.gradientBrush.GradientStops.Add(stop);
			}
		}

		#region Fields

		bool _isHoldingPin = false;
		bool _isDraggingPin = false;
		double _dragStartPosX = 0;

		ViewModels.MyGradientEditorViewModel _viewModel = new ViewModels.MyGradientEditorViewModel();

		#endregion


		/// <summary>
		/// デフォルト コンストラクタ。
		/// </summary>
		public MyGradientEditor()
		{
			InitializeComponent();

			this._viewModel.PropertyChanged += (s, e) =>
			{
				if (e.PropertyName == nameof(ViewModels.MyGradientEditorViewModel.CurrentColorOffsetPercent))
				{
					this.UpdateVisualGradientStops();
				}
			};

			this.buttonDeleteColorStop.Click += (s, e) =>
			{
				this.DeleteCurrentGradientStop();
			};

			// ダミーデータの削除。
			this.gradientBrush.GradientStops.Clear();

			// WPF の GradientBrush.GradientStops の仕様は、Photoshop のグラデーションと近い仕様。
			// 端の色は先頭・末尾の GradientStop でクランプされる。

			//this.DataContext = this._viewModel;
			this.borderColorPalette.DataContext = this._viewModel;
			this.textboxCurrentColorOffsetPercent.DataContext = this._viewModel;
			this.colorStopArea.DataContext = this._viewModel.GradientStops;

			// デフォルトの NaN で Grid の Auto を使ってサイズを決めると、実際のサイズは ActualWidth, ActualHeight で知るほかないが、
			// コンストラクト直後、レンダリングされていない状態では Actual も NaN になってしまう。
			// Width, Height の値は実際の描画サイズではなく論理値だが、
			// グラデーション データの設定や UI 操作ロジックにも関わりがあるので、NaN ではまずい。
			System.Diagnostics.Debug.Assert(!Double.IsNaN(this.colorStopArea.Width) && this.colorStopArea.Width > 0);
			System.Diagnostics.Debug.Assert(!Double.IsNaN(this.colorStopArea.Height) && this.colorStopArea.Height > 0);
		}

		void SetActiveGradientStop(ViewModels.MyGradientStopViewModel target)
		{
			foreach (var stop in this._viewModel.GradientStops)
			{
				System.Diagnostics.Debug.Assert(stop != null);
				stop.IsSelected = (stop == target);
			}
			this._viewModel.CurrentGradientStop = target;
		}

		void UpdateVisualGradientStops()
		{
			// 自作の VM は直接プロパティを書き換えても OK だが、
			// GradientBrush.GradientStops は GradientStop を入れ替える必要がある。
			if (this.gradientBrush.GradientStops.Remove(this._viewModel.CurrentGradientStop.Tag as GradientStop))
			{
				var stop = CreateGradientStopFromVM(this._viewModel.CurrentGradientStop);
				this._viewModel.CurrentGradientStop.Tag = stop;
				this.gradientBrush.GradientStops.Add(stop);
			}
		}

		void UpdateCurrentColorLocationText()
		{
			var be = this.textboxCurrentColorOffsetPercent.GetBindingExpression(TextBox.TextProperty);
			be.UpdateTarget();
		}

		void DeleteCurrentGradientStop()
		{
			if (this._viewModel.CurrentGradientStop != null)
			{
				this._viewModel.GradientStops.Remove(this._viewModel.CurrentGradientStop);
				this.gradientBrush.GradientStops.Remove(this._viewModel.CurrentGradientStop.Tag as GradientStop);
				this._viewModel.CurrentGradientStop = new ViewModels.MyGradientStopViewModel();
			}
		}

		[System.Diagnostics.Conditional("DEBUG")]
		private void DebugWriteCallerMethodName([System.Runtime.CompilerServices.CallerMemberName] string member = "")
		{
			System.Diagnostics.Debug.WriteLine(member);
		}

		private void colorStopArea_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
			DebugWriteCallerMethodName();
			//System.Diagnostics.Debug.WriteLine(System.Reflection.MethodBase.GetCurrentMethod().Name);
			if (e.ClickCount == 1 && !this._isHoldingPin)
			{
				var newVM = new ViewModels.MyGradientStopViewModel();
				double newPosX = e.GetPosition(this.colorStopArea).X;
				this._viewModel.GradientStops.Add(newVM);
				var normalizedOffset = NormalClamp(newPosX / this.colorStopArea.Width);
				// 追加する色は、現在選択中のパレット色となる。作成中のグラデーションから補間した色ではない。
				var newColor = this._viewModel.CurrentGradientStop != null ? this._viewModel.CurrentGradientStop.Color : Colors.White;
				newVM.Offset = normalizedOffset;
				newVM.Color = newColor;
				var stop = CreateGradientStopFromVM(newVM);
				newVM.Tag = stop;
				this.gradientBrush.GradientStops.Add(stop);
				this.SetActiveGradientStop(newVM);
			}
		}

		private void colorStopArea_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
		{
			DebugWriteCallerMethodName();
			if (this._isHoldingPin)
			{
				// 終了位置の上下がはみ出している場合、グラデーション ストップを消去。
				double endPosY = e.GetPosition(this.colorStopArea).Y;
				if (endPosY < 0 || this.colorStopArea.Height < endPosY)
				{
					this.DeleteCurrentGradientStop();
				}
				Mouse.Capture(null);
				this._isHoldingPin = false;
			}
		}

		private void colorStopArea_MouseMove(object sender, MouseEventArgs e)
		{
			if (!this._isHoldingPin)
			{
				return;
			}

			if (Math.Abs(this._dragStartPosX - e.GetPosition(this.colorStopArea).X) >= 2)
			{
				this._isDraggingPin = true;
			}

			if (this._isDraggingPin)
			{
				double newPosX = e.GetPosition(this.colorStopArea).X;
				//Debug.WriteLine("newPosX = {0}", newPosX);
				var normalizedOffset = NormalClamp(newPosX / this.colorStopArea.Width);
				this._viewModel.CurrentGradientStop.Offset = normalizedOffset;
				this.UpdateVisualGradientStops();
				this.UpdateCurrentColorLocationText();
				// 現在位置の上下がはみ出している場合、グラデーション ストップを消去する予告として、半透明にする。
				double endPosY = e.GetPosition(this.colorStopArea).Y;
				if (endPosY < 0 || this.colorStopArea.Height < endPosY)
				{
					this._viewModel.CurrentGradientStop.PreDeletion = true;
				}
				else
				{
					this._viewModel.CurrentGradientStop.PreDeletion = false;
				}
			}
		}

		private void colorStopArea_MouseLeave(object sender, MouseEventArgs e)
		{
			DebugWriteCallerMethodName();
		}

		private void MyGradientStopPin_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
			DebugWriteCallerMethodName();
			if (e.ClickCount == 1)
			{
				// ダブルクリックすると、2回 MouseDown イベントが発生する。そのうち、2回目は MouseDoubleClick イベントの後に発生する。
				var stopPin = sender as MyGradientStopPin;
				var vm = stopPin.DataContext as ViewModels.MyGradientStopViewModel;
				System.Diagnostics.Debug.Assert(vm != null);
				this.SetActiveGradientStop(vm);
				Mouse.Capture(stopPin);
				this._isHoldingPin = true;
				this._isDraggingPin = false;
				this._dragStartPosX = e.GetPosition(this.colorStopArea).X;
			}
		}

		private void MyGradientStopPin_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
		{
			DebugWriteCallerMethodName();
			//Mouse.Capture(null);
			//this._isHoldingPin = false;
		}

		private void MyGradientStopPin_MouseDoubleClick(object sender, MouseButtonEventArgs e)
		{
			DebugWriteCallerMethodName();
			if (e.ChangedButton == MouseButton.Left)
			{
				ShowColorDialog();
			}
		}

		private void borderColorPalette_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
			ShowColorDialog();
		}

		private void ShowColorDialog()
		{
			// HACK: Windows ストア アプリや Windows Phone などにも移植する場合、Windows Forms (Win32) のカラーダイアログは使えない。
			// .NET Compact Framework でもサポートされていないらしい。
			// ストア アプリでもカラーピッカーは用意されていないようなので、必要に応じて
			// Photoshop ライクの RGB/HSV/HSB カラーエディタなどを改めて XAML 実装したほうがよい。

			this._isHoldingPin = false;
			var vm = this._viewModel.CurrentGradientStop;
			var colorDlg = new System.Windows.Forms.ColorDialog();
			var brushColor = vm != null ? vm.Color : Colors.White;
			colorDlg.Color = System.Drawing.Color.FromArgb(brushColor.R, brushColor.G, brushColor.B);
			colorDlg.FullOpen = true;
			if (colorDlg.ShowDialog() == System.Windows.Forms.DialogResult.OK)
			{
				var newColor = Color.FromRgb(colorDlg.Color.R, colorDlg.Color.G, colorDlg.Color.B);
				if (vm != null)
				{
					vm.Color = newColor;
					this.SetActiveGradientStop(vm);
					var stop = vm.Tag as GradientStop;
					if (stop != null)
					{
						stop.Color = vm.Color;
					}
				}
			}
		}
	}

	internal sealed class MultiplyDouble2Converter : IMultiValueConverter
	{
		public object Convert(object[] values, Type targetType, object parameter, System.Globalization.CultureInfo culture)
		{
			if (!(values[0] is double))
			{
				throw new ArgumentException("values[0] must be a 'double'!!");
			}
			if (!(values[1] is double))
			{
				throw new ArgumentException("values[1] must be a 'double'!!");
			}
			return (double)values[0] * (double)values[1];
		}

		public object[] ConvertBack(object value, Type[] targetTypes, object parameter, System.Globalization.CultureInfo culture)
		{
			throw new NotImplementedException();
		}
	}

	namespace ViewModels
	{
		public class MyGradientEditorViewModel : MyBindingHelpers.MyNotifyPropertyChangedBase
		{
			public readonly ObservableCollection<MyGradientStopViewModel> GradientStops = new ObservableCollection<MyGradientStopViewModel>();

			MyGradientStopViewModel _currentGradientStop = new MyGradientStopViewModel();

			public MyGradientStopViewModel CurrentGradientStop
			{
				get { return this._currentGradientStop; }
				set
				{
					System.Diagnostics.Debug.Assert(value != null);
					if (base.SetSingleProperty(ref this._currentGradientStop, value))
					{
						this.NotifyPropertyChanged(nameof(this.CurrentColorOffsetPercent));
					}
				}
			}

			public double CurrentColorOffsetPercent
			{
				get { return this._currentGradientStop.Offset * 100; }
				set
				{
					// TODO: ビューから VM を書き換えたときに、GradientBrush.GradientStops の更新が必要。
					double temp = this._currentGradientStop.Offset;
					if (base.SetSingleProperty(ref temp, value * 0.01))
					{
						this._currentGradientStop.Offset = temp;
					}
				}
			}
		}
	}
}
