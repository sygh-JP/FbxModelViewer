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
using System.Windows.Shapes;

namespace MyWpfGraphLibrary
{
	/// <summary>
	/// MyTaskProgressDialog.xaml の相互作用ロジック
	/// </summary>
	public partial class MyTaskProgressDialog : Window
	{
		bool _isEnforcedClosing = false;

		public MyTaskProgressDialog()
		{
			InitializeComponent();

			//this.ContentRendered += MyTaskProgressDialog_ContentRendered;
			this.Closing += MyTaskProgressDialog_Closing;
		}

		private void MyTaskProgressDialog_Closing(object sender, System.ComponentModel.CancelEventArgs e)
		{
			if (!this._isEnforcedClosing)
			{
				e.Cancel = true;
			}
		}

		public void EnforcedClose()
		{
			this._isEnforcedClosing = true;
			//this.Dispatcher.Invoke(() => this.Close());
			this.Close();
		}

		public bool OnceContentRendered { get; set; }

		public event Action MainWorkStarted;

		private async void MyTaskProgressDialog_ContentRendered(object sender, EventArgs e)
		{
			// マネージ サブスレッド上で、指定した処理を非同期に開始する。
			await Task.Run(MainWorkStarted);
			// タスクが終了すれば、そのままウィンドウを閉じる。
			this.EnforcedClose();
		}
	}
}
