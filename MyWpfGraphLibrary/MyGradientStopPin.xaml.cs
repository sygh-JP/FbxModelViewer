using System;
using System.Collections.Generic;
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

namespace MyWpfGraphLibrary
{
	/// <summary>
	/// MyGradientStopPin.xaml の相互作用ロジック
	/// </summary>
	public partial class MyGradientStopPin : UserControl
	{
		/// <summary>
		/// デフォルト コンストラクタ。
		/// </summary>
		public MyGradientStopPin()
		{
			InitializeComponent();
		}

		private bool f_isActivePin = false;

		//private GradientStop f_associatedGradientStop;

		public GradientStop AssociatedGradientStop { get; protected set; }

		SolidColorBrush TipTriangleFillBrush
		{
			get
			{
				return this.tipTriangle.Fill as SolidColorBrush;
			}
			set
			{
				this.tipTriangle.Fill = value;
			}
		}

		public SolidColorBrush SurfaceRectFillBrush
		{
			get
			{
				return this.surfaceRect.Fill as SolidColorBrush;
			}
			protected set
			{
				this.surfaceRect.Fill = value;
			}
		}

		public Color SurfaceRectFillBrushColor
		{
			get
			{
				return this.SurfaceRectFillBrush.Color;
			}
		}

		public bool IsActivePin
		{
			get
			{
				return f_isActivePin;
			}
			set
			{
				f_isActivePin = value;
				this.TipTriangleFillBrush = value ? Brushes.Black : Brushes.White;
			}
		}

		public void SetSurfaceColor(Color color)
		{
			this.SurfaceRectFillBrush = new SolidColorBrush(color);
			// 読み取り専用（定義済み）の Brushes メンバーなどが割り当てられている場合、
			// SolidColorBrush.Color プロパティを直接変更できない。
			// 同値を持つ SolidColorBrush オブジェクトの再割り当てが必要。
			Debug.Assert(this.AssociatedGradientStop != null);
			this.AssociatedGradientStop.Color = color;
		}

		public void SetGradientOffset(double offsetVal)
		{
			// Color プロパティは R/W だが、Offset プロパティは読み取り専用なので、再割り当てが必要。
			Debug.Assert(this.AssociatedGradientStop != null);
			this.AssociatedGradientStop = new GradientStop(this.AssociatedGradientStop.Color, offsetVal);
		}

		public void SetSurfaceColorAndGradientOffset(Color color, double offsetVal)
		{
			this.SurfaceRectFillBrush = new SolidColorBrush(color);
			// Color プロパティは R/W だが、Offset プロパティは読み取り専用なので、再割り当てが必要。
			this.AssociatedGradientStop = new GradientStop(color, offsetVal);
		}
	}
}
