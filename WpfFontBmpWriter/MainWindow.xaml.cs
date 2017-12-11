using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using Microsoft.Win32;

//using System.Windows.Controls.Ribbon;
// WPF 3.5 時代では Microsoft.Windows.Controls.Ribbon だったが、WPF 4.5 では正式に System 名前空間配下に取り込まれている。
// なお、利用するには同名のアセンブリを参照設定に追加する必要がある。
// ただ、Windows 7 上では RibbonWindow の WindowChrome の高さが小さすぎたり、
// RibbonQuickAccessToolBar の直下にあるパネルの高さが小さすぎたりして、
// 見栄えがよくない。カスタマイズが必要そう。

namespace WpfFontBmpWriter
{
	/// <summary>
	/// MainWindow.xaml の相互作用ロジック
	/// </summary>
	public partial class MainWindow : Window
	{
		public MainWindow()
		{
			InitializeComponent();

			for (int i = MinImageSize; i <= MaxImageSize; i *= 2)
			{
				this.comboImageWidth.Items.Add(i.ToString());
				this.comboImageHeight.Items.Add(i.ToString());
			}
		}

		const int MinImageSize = 8;
		//const int MaxImageSize = 2048;
		const int MaxImageSize = 4096;
		// Direct3D Feature Level 9_1 世代での 1D/2D テクスチャ サイズ（横もしくは縦）の上限が 2048、
		// Feature Level 9_3 世代での上限が 4096 となっている。
		// DirectX 10 対応ハードウェアでは 8192、DirectX 11 対応ハードウェアでは 16384 までいける。
		// 下限は常に1だが、フォント画像を作るのに1はありえないので適当なサイズを下限とする。

		private void buttonSave_Click(object sender, RoutedEventArgs e)
		{
			try
			{
				var fileDlg = new SaveFileDialog();
				fileDlg.DefaultExt = "*.png";
				fileDlg.Filter = "PNG(*.png)|*.png";
				if (fileDlg.ShowDialog() == true)
				{
					this.fontImageView1.SaveAsPngFile(fileDlg.FileName);
					this.fontImageView1.SaveUVMapAsCsvFile(fileDlg.FileName + ".csv");
				}
			}
			catch (Exception err)
			{
				MessageBox.Show(err.Message, "Image Save Error", MessageBoxButton.OK, MessageBoxImage.Error);
			}
		}

		private void mainWindow_Loaded(object sender, RoutedEventArgs e)
		{
			// デフォルト値の設定。
			// HACK: 前回の設定値をストレージに記憶しておくとよいかも。
			this.fontImageView1.FontSize = 16;
			this.fontImageView1.FontFamily = new FontFamily("Consolas");
			this.fontImageView1.FontWeight = FontWeights.Normal;

			this.comboFonts.Text = this.fontImageView1.FontFamily.Source;
		}

		private void buttonRefresh_Click(object sender, RoutedEventArgs e)
		{
			this.fontImageView1.FontFamily = new FontFamily(this.comboFonts.Text);
			int imageWidth, imageHeight;
			if (
				!Int32.TryParse(this.comboImageWidth.Text, out imageWidth) ||
				!Int32.TryParse(this.comboImageHeight.Text, out imageHeight))
			{
				MessageBox.Show("画像サイズは整数値を入力してください。", MyWpfHelpers.MyWpfMiscHelper.GetAppName(), MessageBoxButton.OK, MessageBoxImage.Error);
				return;
			}
			if (imageWidth <= MinImageSize || MaxImageSize < imageWidth ||
				imageHeight <= MinImageSize || MaxImageSize < imageHeight)
			{
				MessageBox.Show("画像サイズが範囲外です。", MyWpfHelpers.MyWpfMiscHelper.GetAppName(), MessageBoxButton.OK, MessageBoxImage.Error);
				return;
			}
			this.fontImageView1.UpdateFontBitmap(imageWidth, imageHeight);
		}
	}
}


namespace MyMiscHelpers
{
	public static class MyMathHelper
	{
		/// <summary>
		/// x が 2 のべき乗であるか否かを調べる。
		/// </summary>
		/// <param name="x"></param>
		/// <returns></returns>
		public static bool IsModulo2(uint x)
		{ return x != 0 && (x & (x - 1)) == 0; }
	}
}

namespace MyWpfHelpers
{
	public static class MyWpfMiscHelper
	{
		public static string GetAppName()
		{
			return Application.ResourceAssembly.GetName().Name;
			// Assembly.GetExecutingAssembly().GetName().Name はコード実行中のアセンブリの名前を取得するものであることに注意。
		}
	}
}
