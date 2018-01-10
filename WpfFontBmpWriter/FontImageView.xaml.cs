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

namespace WpfFontBmpWriter
{
	/// <summary>
	/// FontImageView.xaml の相互作用ロジック
	/// </summary>
	public partial class FontImageView : UserControl
	{
		readonly char[] f_asciiArray = CreateAsciiArray();
		Rect[] f_uvRectArray;

		const int DefaultBmpWidth = 256;
		const int DefaultBmpHeight = 256;


		public FontImageView()
		{
			InitializeComponent();
		}

		private static char[] CreateAsciiArray()
		{
			// ASCII のみ。
			// TODO: 日本語対応する（かな・カナ・記号・漢字など）。
			// 漢字のフルサポートはサロゲート ペアの問題もあり厳しい。ゲーム用途では第1～第2水準漢字程度があれば十分。
			// Unicode フル対応するには Direct2D & DirectWrite を使ったほうがよい。
			const int firstCode = 0x20;
			const int lastCode = 0x7e;
			const int codeCount = lastCode - firstCode + 1;
			var charsArray = new char[codeCount];
			for (var i = 0; i < codeCount; ++i)
			{
				charsArray[i] = (char)(firstCode + i);
			}
			return charsArray;
		}

		private void UserControl_Loaded(object sender, RoutedEventArgs e)
		{
			this.UpdateFontBitmap(DefaultBmpWidth, DefaultBmpHeight);
		}

		public void UpdateFontBitmap(int width, int height)
		{
			this.Width = width;
			this.Height = height;
			RenderFontBitmap(width, height, out f_uvRectArray);
		}

		private void RenderFontBitmap(int bmpWidth, int bmpHeight, out Rect[] uvRectArray)
		{
			// RenderTargetBitmap を使うと、ClearType アンチエイリアスではなくグレースケール アンチエイリアスになるらしい。
			// アルファ マップを作成するのが目的であれば、むしろ好都合。
			// http://social.msdn.microsoft.com/Forums/ja-JP/wpffaqja/thread/df0c59a1-f7c0-4591-9285-eeabc252a608/

			Debug.Assert(f_asciiArray.Length > 0);
			uvRectArray = new Rect[f_asciiArray.Length];

			// RenderTargetBitmap へのオフスクリーン描画の際は、システム DPI ではなく、RenderTargetBitmap の DPI が使われるようになる模様。
			// これを使えば、複数の DPI パターンのフォント画像を事前生成できる。
			const int bmpDpi = 96;
			//const int bmpDpi = 144;
			var target = new RenderTargetBitmap(bmpWidth, bmpHeight, bmpDpi, bmpDpi, PixelFormats.Pbgra32);
			var dv = new DrawingVisual();
			// DrawingContext を使って描画。
			using (var dc = dv.RenderOpen())
			{
				//dc.DrawRectangle(new SolidColorBrush(Color.FromArgb(128, 20, 20, 255)), null, new Rect(0, 0, DefaultBmpWidth, DefaultBmpHeight));

				const double initX = 2, initY = 0;
				double posX = initX, posY = initY;
				double maxFontHeight = 0;
				// 1文字ずつバッファに書き込んでいく。
				for (int i = 0; i < f_asciiArray.Length; ++i)
				{
					var c = f_asciiArray[i];

					// ネイティブ DirectWrite の IDWriteTextLayout による描画の流れとよく似ている。
					// FormattedText のプロパティは DWRITE_TEXT_METRICS のメンバーと酷似している。

					var formattedText = new FormattedText(
						//"Drawing sample!",
						c.ToString(),
						System.Globalization.CultureInfo.CurrentUICulture,
						FlowDirection.LeftToRight,
						new Typeface(this.FontFamily.Source),
						this.FontSize, Brushes.White);

					const double paddingX = 2, paddingY = 1;
					//double fontWidth = formattedText.WidthIncludingTrailingWhitespace;
					//double fontWidth = formattedText.Width + formattedText.WidthIncludingTrailingWhitespace;
					//double fontWidth = (formattedText.OverhangTrailing > 0 ? formattedText.OverhangTrailing : 0) + formattedText.WidthIncludingTrailingWhitespace;
					//double fontWidth = formattedText.MinWidth;
					double fontWidth = Math.Ceiling(formattedText.WidthIncludingTrailingWhitespace);
					double fontHeight = Math.Ceiling(formattedText.Height);
					double feedX = fontWidth + paddingX;
					double feedY = fontHeight + paddingY;
					maxFontHeight = Math.Max(maxFontHeight, fontHeight);
					double devPixelsPerLogX = target.DpiX / 96;
					double devPixelsPerLogY = target.DpiY / 96;
					if ((posX + feedX) * devPixelsPerLogX > bmpWidth)
					{
						posX = initX;
						posY += maxFontHeight + paddingY;
					}

					dc.DrawText(formattedText, new Point(posX, posY));

					// 1文字分のフォント境界ボックス矩形の UV を計算する。
					// TODO: イタリック体への対応はどうする？
					// スプライト フォントの場合、立体をアプリ側で機械的に傾ける（オブリーク体）ほうが楽ではある。
					// ただし Times New Roman のように、イタリック体では字形が変わるものもある。
					// HACK: Times New Roman Italic のダブルクォーテーションの境界矩形が小さくなりすぎる（隣にオーバーラップする？）問題がある。
					var uvRect = new Rect(posX * devPixelsPerLogX, posY * devPixelsPerLogY, fontWidth * devPixelsPerLogX, fontHeight * devPixelsPerLogY);
					uvRectArray[i] = uvRect;

					posX += feedX;
				}
			}
			target.Render(dv);
			this.mainImage.Source = target;
		}

		public void SaveAsPngFile(string filePath)
		{
			// 単にフォントのアルファ マップが欲しい場合、アルファ チャンネルを抽出してグレースケール PNG で保存したほうがさらに容量を節約できる。
			var encoder = new PngBitmapEncoder();
			encoder.Frames.Add(BitmapFrame.Create(this.mainImage.Source as BitmapSource));
			using (var stream = System.IO.File.Create(filePath))
			{
				encoder.Save(stream);
			}
		}

		public void SaveUVMapAsCsvFile(string filePath)
		{
			if (f_asciiArray.Length != f_uvRectArray.Length)
			{
				throw new Exception("No UV data!!");
			}
			using (var sw = new System.IO.StreamWriter(filePath, false, new UTF8Encoding(true)))
			{
				sw.WriteLine("# UV Coord Map");
				sw.WriteLine("# UCS-2, X, Y, Width, Height");
				for (int i = 0; i < f_asciiArray.Length; ++i)
				{
					var uv = f_uvRectArray[i];
					sw.WriteLine(String.Format("0x{0:X4}, {1}, {2}, {3}, {4}",
						(int)f_asciiArray[i],
						uv.X, uv.Y, uv.Width, uv.Height));
				}
			}
		}
	}
}
