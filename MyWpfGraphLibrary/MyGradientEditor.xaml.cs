using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
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

// グラデーション オフセットの UI 設定値は 0[%]～100[%] だが、内部では 0.0～1.0 の正規化された値で管理する。


namespace MyWpfGraphLibrary
{
	/// <summary>
	/// MyGradientEditor.xaml の相互作用ロジック
	/// </summary>
	public partial class MyGradientEditor : UserControl, INotifyPropertyChanged
	{
		public interface IManagedEventListener
		{
			//void OnUpdate(MyGradientStopPin[] gradientStopPins);
			void OnUpdate(int gradientStopCount);
		}

		#region Properties

		public MyGradientEditor.IManagedEventListener EventListener { set; get; }
		public bool CanNotifyListener { get { return this.EventListener != null; } }

		public double CurrentColorLocation
		{
			get
			{
				if (f_currentColorStopPin != null)
				{
					return f_currentColorStopPin.AssociatedGradientStop.Offset * 100;
				}
				else
				{
					return 0;
				}
			}
			protected set
			{
				if (f_currentColorStopPin != null)
				{
					double normalizedOffset = MyMiscHelpers.MyGenericsHelper.Clamp(value * 0.01, 0.0, 1.0);
					Canvas.SetLeft(f_currentColorStopPin, normalizedOffset * this.colorStopArea.Width);
					// GradientStop を入れ替える。
					f_gradientBrush.GradientStops.Remove(f_currentColorStopPin.AssociatedGradientStop);
					f_currentColorStopPin.SetGradientOffset(normalizedOffset);
					f_gradientBrush.GradientStops.Add(f_currentColorStopPin.AssociatedGradientStop);
					// 普通はプロパティ名の文字列リテラルを直接記述すればよいが、あえてメンテナンス性を考慮して式木からプロパティ名を取得する。
					this.NotifyPropertyChanged(MyMiscHelpers.MyGenericsHelper.GetMemberName(() => this.CurrentColorLocation));
				}
			}
		}

		public SolidColorBrush ColorPaletteBrush
		{
			get
			{
				return this.borderColorPalette.Background as SolidColorBrush;
			}
			protected set
			{
				this.borderColorPalette.Background = value;
			}
		}

		public Color ColorPaletteBrushColor
		{
			get
			{
				return this.ColorPaletteBrush.Color;
			}
			protected set
			{
				this.ColorPaletteBrush = new SolidColorBrush(value);
				// 読み取り専用（定義済み）の Brushes メンバーなどが割り当てられている場合、
				// SolidColorBrush.Color プロパティを直接変更できない。
				// 同値を持つ SolidColorBrush オブジェクトの明示的作成が必要。
			}
		}

		public MyGradientStopPin[] GetGradientColorStopPinsArray()
		{
			// グラデーション Offset の昇順に並べ替えて配列化したものを返す。
			var query = from pin in this.colorStopArea.Children.OfType<MyGradientStopPin>()
						orderby pin.AssociatedGradientStop.Offset ascending
						select pin;

			return query.ToArray();
		}

		public void SetGradientColorStopPinsArray(MyGradientStopPin[] stopPins)
		{
			// スワップの実装は VisualParent の関係からちょっと大変。全削除して全追加の手順をとる。
			this.colorStopArea.Children.Clear();
			f_gradientBrush.GradientStops.Clear();
			f_currentColorStopPin = null;
			f_isHoldingPin = false;
			f_isDraggingPin = false;
			foreach (var newStop in stopPins)
			{
				// ソートする必要はない。
				double normalizedOffset = MyMiscHelpers.MyGenericsHelper.Clamp(newStop.AssociatedGradientStop.Offset, 0.0, 1.0);
				Canvas.SetLeft(newStop, normalizedOffset * this.colorStopArea.Width);
				newStop.MouseLeftButtonDown += MyGradientStopPin_MouseLeftButtonDown;
				newStop.MouseLeftButtonUp += MyGradientStopPin_MouseLeftButtonUp;
				newStop.MouseDoubleClick += MyGradientStopPin_MouseDoubleClick;
				this.colorStopArea.Children.Add(newStop);
				f_gradientBrush.GradientStops.Add(newStop.AssociatedGradientStop);
				this.UpdateColorStopCountLabel();
			}
		}

		#endregion

		#region Fields

		bool f_isHoldingPin = false;
		bool f_isDraggingPin = false;
		double f_dragStartPosX = 0;
		MyGradientStopPin f_currentColorStopPin;
		LinearGradientBrush f_gradientBrush;

		public event PropertyChangedEventHandler PropertyChanged;

		#endregion


		/// <summary>
		/// デフォルト コンストラクタ。
		/// </summary>
		public MyGradientEditor()
		{
			InitializeComponent();

			f_gradientBrush = this.borderGradient.Background as LinearGradientBrush;
			f_gradientBrush.GradientStops.Clear();

			this.colorStopArea.Children.Clear(); // ダミーデータの削除。
			this.UpdateColorStopCountLabel();

			// HACK: MVVM 的には邪道。View と ViewModel をきちんと分離する。
			this.DataContext = this;

			// デフォルトの NaN で Grid の Auto を使ってサイズを決めると、実際のサイズは ActualWidth, ActualHeight で知るほかないが、
			// コンストラクト直後、レンダリングされていない状態では Actual～ も NaN になってしまう。
			// Width, Height の値は実際の描画サイズではなく論理値だが、
			// グラデーション データの設定や UI 操作ロジックにも関わりがあるので、NaN ではまずい。
			Debug.Assert(!Double.IsNaN(this.colorStopArea.Width) && this.colorStopArea.Width > 0);
			Debug.Assert(!Double.IsNaN(this.colorStopArea.Height) && this.colorStopArea.Height > 0);
		}

		void NotifyPropertyChanged(string propName)
		{
			if (this.PropertyChanged != null)
			{
				this.PropertyChanged(this, new PropertyChangedEventArgs(propName));
			}
		}

		void UpdateColorStopCountLabel()
		{
			//this.labelColorStopCount.Content = this.colorStopArea.Children.Count;
			var be = this.labelColorStopCount.GetBindingExpression(Label.ContentProperty);
			be.UpdateTarget();
		}

		void UpdateCurrentColorLocationText()
		{
			var be = this.textboxCurrentColorLocation.GetBindingExpression(TextBox.TextProperty);
			be.UpdateTarget();
		}

		void SetActiveGradientStop(MyGradientStopPin target)
		{
			foreach (var child in this.colorStopArea.Children)
			{
				var stop = child as MyGradientStopPin;
				Debug.Assert(stop != null);
				stop.IsActivePin = (stop == target);
			}
			this.ColorPaletteBrush = target.SurfaceRectFillBrush;
			f_currentColorStopPin = target;

			this.UpdateCurrentColorLocationText();
		}

		private void colorStopArea_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
			Debug.WriteLine(System.Reflection.MethodBase.GetCurrentMethod().Name);
			if (e.ClickCount == 1 && !f_isHoldingPin)
			{
				var newStop = new MyGradientStopPin();
				double newPosX = e.GetPosition(this.colorStopArea).X;
				Canvas.SetLeft(newStop, newPosX);
				newStop.MouseLeftButtonDown += MyGradientStopPin_MouseLeftButtonDown;
				newStop.MouseLeftButtonUp += MyGradientStopPin_MouseLeftButtonUp;
				newStop.MouseDoubleClick += MyGradientStopPin_MouseDoubleClick;
				this.colorStopArea.Children.Add(newStop);
				this.UpdateColorStopCountLabel();
				var normalizedOffset = MyMiscHelpers.MyGenericsHelper.Clamp(newPosX / this.colorStopArea.Width, 0.0, 1.0);
				// 追加する色は、現在選択中のパレット色となる。作成中のグラデーションから補間した色ではない。
				var currPaletteBgBrush = this.ColorPaletteBrush as SolidColorBrush;
				Debug.Assert(currPaletteBgBrush != null);
				//var newColor = Colors.White;
				var newColor = currPaletteBgBrush != null ? currPaletteBgBrush.Color : Colors.White;
				newStop.SetSurfaceColorAndGradientOffset(newColor, normalizedOffset);
				f_gradientBrush.GradientStops.Add(newStop.AssociatedGradientStop);
				this.SetActiveGradientStop(newStop);
			}
		}

		private void colorStopArea_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
		{
			Debug.WriteLine(System.Reflection.MethodBase.GetCurrentMethod().Name);
			if (f_currentColorStopPin != null)
			{
				// 終了位置の上下がはみ出している場合、グラデーション ストップを消去。
				double endPosY = e.GetPosition(this.colorStopArea).Y;
				if (endPosY < 0 || this.colorStopArea.Height < endPosY)
				{
					this.colorStopArea.Children.Remove(f_currentColorStopPin);
					this.UpdateColorStopCountLabel();
					f_gradientBrush.GradientStops.Remove(f_currentColorStopPin.AssociatedGradientStop);
					f_currentColorStopPin = null;
				}
				Mouse.Capture(null);
				f_isHoldingPin = false;
			}
		}

		private void colorStopArea_MouseMove(object sender, MouseEventArgs e)
		{
			if (!f_isHoldingPin)
			{
				return;
			}

			if (Math.Abs(f_dragStartPosX - e.GetPosition(this.colorStopArea).X) >= 2)
			{
				f_isDraggingPin = true;
			}

			if (f_isDraggingPin)
			{
				double newPosX = e.GetPosition(this.colorStopArea).X;
				//Debug.WriteLine("newPosX = {0}", newPosX);
				this.CurrentColorLocation = 100 * newPosX / this.colorStopArea.Width;
				// 現在位置の上下がはみ出している場合、グラデーション ストップを消去する予告として、半透明にする。
				double endPosY = e.GetPosition(this.colorStopArea).Y;
				if (endPosY < 0 || this.colorStopArea.Height < endPosY)
				{
					f_currentColorStopPin.Opacity = 0.5;
				}
				else
				{
					f_currentColorStopPin.Opacity = 1.0;
				}
			}
		}

		private void colorStopArea_MouseLeave(object sender, MouseEventArgs e)
		{
			Debug.WriteLine(System.Reflection.MethodBase.GetCurrentMethod().Name);
		}

		private void MyGradientStopPin_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
			Debug.WriteLine(System.Reflection.MethodBase.GetCurrentMethod().Name);
			if (e.ClickCount == 1)
			{
				// ダブルクリックすると、2回 MouseDown イベントが発生する。そのうち、2回目は MouseDoubleClick イベントの後に発生する。
				var stopPin = sender as MyGradientStopPin;
				this.SetActiveGradientStop(stopPin);
				Mouse.Capture(stopPin);
				f_isHoldingPin = true;
				f_isDraggingPin = false;
				f_dragStartPosX = e.GetPosition(this.colorStopArea).X;
			}
		}

		private void MyGradientStopPin_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
		{
			Debug.WriteLine(System.Reflection.MethodBase.GetCurrentMethod().Name);
			Mouse.Capture(null);
			f_isHoldingPin = false;
		}

		private void MyGradientStopPin_MouseDoubleClick(object sender, MouseButtonEventArgs e)
		{
			Debug.WriteLine(System.Reflection.MethodBase.GetCurrentMethod().Name);
			if (e.ChangedButton == MouseButton.Left)
			{
				//var stopPin = sender as MyGradientStopPin;
				//Debug.Assert(stopPin != null);
				ShowColorDialog();
				//MessageBox.Show("Left Button is Double-Clicked!!");
			}
		}

		private void borderColorPalette_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
			ShowColorDialog();
		}

		private void ShowColorDialog()
		{
#if false
			if (f_currentColorStopPin == null)
			{
				return;
			}
#endif

			// HACK: Windows ストア アプリや Windows Phone などにも移植する場合、Windows Forms (Win32) のカラーダイアログは使えない。
			// .NET Compact Framework でもサポートされていないらしい。
			// ストア アプリでもカラーピッカーは用意されていないようなので、必要に応じて
			// Photoshop ライクの RGB/HSV/HSB カラーエディタなどを改めて XAML 実装したほうがよい。

			f_isHoldingPin = false;
			var stopPin = f_currentColorStopPin;
			var colorDlg = new System.Windows.Forms.ColorDialog();
			var brushColor = stopPin != null ? stopPin.SurfaceRectFillBrushColor : this.ColorPaletteBrushColor;
			colorDlg.Color = System.Drawing.Color.FromArgb(brushColor.R, brushColor.G, brushColor.B);
			colorDlg.FullOpen = true;
			if (colorDlg.ShowDialog() == System.Windows.Forms.DialogResult.OK)
			{
				var newColor = System.Windows.Media.Color.FromRgb(colorDlg.Color.R, colorDlg.Color.G, colorDlg.Color.B);
				if (stopPin != null)
				{
					stopPin.SetSurfaceColor(newColor);
					this.SetActiveGradientStop(stopPin);
				}
				else
				{
					this.ColorPaletteBrushColor = newColor;
				}
			}
		}
	}
}
