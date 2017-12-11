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
	}

	namespace ViewModels
	{
		public class MyGradientStopViewModel : MyBindingHelpers.MyNotifyPropertyChangedBase
		{
			bool _isSelected = false;
			bool _preDeletion = false;
			Color _color = Colors.White;
			double _offset = 0;

			// HACK: Opacity 専用のプロパティを用意する？　Color に含める？

			public bool IsSelected
			{
				get { return this._isSelected; }
				set { base.SetSingleProperty(ref this._isSelected, value); }
			}

			public bool PreDeletion
			{
				get { return this._preDeletion; }
				set { base.SetSingleProperty(ref this._preDeletion, value); }
			}

			public Color Color
			{
				get { return this._color; }
				set { base.SetSingleProperty(ref this._color, value); }
			}

			public double Offset
			{
				get { return this._offset; }
				set { base.SetSingleProperty(ref this._offset, value); }
			}

			public object Tag { get; set; }
		}
	}

#if false
	public class MySelector : System.Windows.Controls.Primitives.Selector
	{
	}
#endif
}
