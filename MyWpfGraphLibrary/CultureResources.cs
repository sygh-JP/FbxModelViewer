using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.Windows.Data;


// cf.
// http://www.codeproject.com/Articles/22967/WPF-Runtime-Localization
// http://d.hatena.ne.jp/akiramei/20081021/1224601501


namespace MyWpfGraphLibrary
{
	public static class CultureResources
	{
		private static readonly List<CultureInfo> f_cultures = new List<CultureInfo>();

		static CultureResources()
		{
			// 言語の選択候補として日本語(日本)と英語(米国)を追加する。
			f_cultures.Add(CultureInfo.GetCultureInfo("ja-JP"));
			f_cultures.Add(CultureInfo.GetCultureInfo("en-US"));

			// C# プロジェクト ファイル（.csproj）の最初の <PropertyGroup> タグ内に、
			// <SupportedCultures>en-US;ja-JP;</SupportedCultures> を手動追加するのを忘れないように。
			// VS 2008 / 2010 には今のところ IDE の UI で設定する機能はない模様。
		}

		public static IList<CultureInfo> Cultures
		{
			get { return f_cultures; }
		}


		internal static Properties.Resources GetResourceInstance()
		{
			// resx ファイルから自動生成されたクラスのインスタンスを返す。
			// VC# 2008 では public でもコンパイルが通っていたが、VC# 2012 では internal でないとコンパイル エラーになる。
			return new Properties.Resources();
		}

		/// <summary>
		/// アプリケーション リソース プロバイダーのシングルトン インスタンス。
		/// </summary>
		private static ObjectDataProvider f_appResProvider;

		private static ObjectDataProvider AppResourceProvider
		{
			get
			{
				// キー "MyResourcesInstance" が App.xaml 内で定義されている場合。
				// 純粋な WPF アプリケーションではこの方法が使えるが、Win32 / MFC や WinForms と相互運用する場合には直接使えない。
				if (f_appResProvider == null && System.Windows.Application.Current != null)
				{
					f_appResProvider = (ObjectDataProvider)System.Windows.Application.Current.FindResource("MyResourcesInstance");
				}
				if (f_appResProvider == null)
				{
					Debug.WriteLine("Failed to find AppResourceProvider!!");
					Debug.Assert(false);
				}
				return f_appResProvider;
			}
		}

		/// <summary>
		/// 言語の明示的切替メソッド。Window や UserControl のリソースを使う場合。
		/// </summary>
		/// <param name="newCulture">新しいカルチャ。</param>
		/// <param name="resProvider">Window や UserControl のリソース プロバイダー。</param>
		public static void ChangeCulture(CultureInfo newCulture, ObjectDataProvider resProvider)
		{
			Debug.Assert(newCulture != null);
			SetResoucesCulture(newCulture);
			Debug.Assert(resProvider != null);
			resProvider.Refresh();
		}

		/// <summary>
		/// 言語の明示的切替メソッド。Application のリソースを使う場合。
		/// </summary>
		/// <param name="newCulture">新しいカルチャ。</param>
		public static void ChangeCulture(CultureInfo newCulture)
		{
			Debug.Assert(newCulture != null);
			SetResoucesCulture(newCulture);
			AppResourceProvider.Refresh();
		}

		public static void SetResoucesCulture(CultureInfo newCulture)
		{
			Properties.Resources.Culture = newCulture;
		}

		// 明示的に System.Windows.Application あるいはその派生クラスをインスタンス化すると、
		// Win32 / MFC や WinForms との相互運用アプリでも System.Windows.Application.Current が自動設定される。
#if false
		private static System.Windows.Application resApp;

		public static void EnsureApplicationResources()
		{
			if (System.Windows.Application.Current == null)
			{
				// create the Application object
				resApp = new System.Windows.Application();

				// merge in your application resources
				System.Windows.Application.Current.Resources.MergedDictionaries.Add(
					System.Windows.Application.LoadComponent(
						new Uri("MyWpfGraphLibrary;component/res/MyResDictionary.xaml",
						UriKind.Relative)) as System.Windows.ResourceDictionary);
			}
		}
#endif
	}
}
