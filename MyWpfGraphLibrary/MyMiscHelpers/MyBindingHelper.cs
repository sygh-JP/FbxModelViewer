using System;
using System.Collections.Generic;
//using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;


namespace MyBindingHelpers
{
	/// <summary>
	/// INotifyPropertyChanged の実装を補助する抽象クラス。
	/// </summary>
	public abstract class MyNotifyPropertyChangedBase : System.ComponentModel.INotifyPropertyChanged
	{
		public event System.ComponentModel.PropertyChangedEventHandler PropertyChanged;
		// Target から Source へのバインディングだけであれば、INotifyPropertyChanged を実装する必要はない。また、自動プロパティでも OK。
		// Source から Target へのバインディングもサポートして双方向バインディングするためには、
		// INotifyPropertyChanged の実装が必須であり、また自動プロパティが使えないので注意。
		// なお、OnPropertyChanged() 仮想メソッドの個別実装は骨が折れるので、System.Linq.Expressions によるバインディング ヘルパーを使う。
		// 普通はプロパティ名の文字列リテラルを直接記述すればよいが、あえてメンテナンス性を考慮して式木からプロパティ名を取得する。
		// System.Linq.Expressions.Expression にラムダ式を渡すことで、プロパティの文字列表現を取得できる。
		// C# 5.0 であれば、Caller Info 属性を使うともっと簡潔かつ効率的に実装できそう。
		// Caller Info は C/C++ の __FILE__ や __LINE__ 同様コンパイル時に処理されるので、リフレクションよりも実行効率がよい。
		// C# 6.0 で導入された nameof 演算子を使えれば一番良いのだが……

		protected void NotifyPropertyChanged(string propertyName)
		{
			var eventHandler = this.PropertyChanged;
			if (eventHandler != null)
			{
				eventHandler(this, new System.ComponentModel.PropertyChangedEventArgs(propertyName));
			}
		}

		/// <summary>
		/// ラムダ式からプロパティの文字列表現を取り出す。C# 6.0 では不要。
		/// </summary>
		/// <typeparam name="T"></typeparam>
		/// <param name="propertyNameExpression"></param>
		protected void NotifyPropertyChanged<T>(System.Linq.Expressions.Expression<Func<T>> propertyNameExpression)
		{
			this.NotifyPropertyChanged(MyMiscHelpers.MyGenericsHelper.GetMemberName(propertyNameExpression));
		}

		/// <summary>
		/// プロパティの setter 内で呼び出すこと。
		/// </summary>
		/// <param name="propertyName"></param>
		protected void RaisePropertyChanged([System.Runtime.CompilerServices.CallerMemberName] string propertyName = "")
		{
			this.NotifyPropertyChanged(propertyName);
		}

		/// <summary>
		/// 値に変更があった場合のみ通知を行なう。
		/// プロパティの setter 内で呼び出すこと。
		/// </summary>
		/// <typeparam name="T"></typeparam>
		/// <param name="dst"></param>
		/// <param name="src"></param>
		/// <param name="propertyName"></param>
		/// <returns>変更があったか否か。</returns>
		protected bool SetSingleProperty<T>(ref T dst, T src, [System.Runtime.CompilerServices.CallerMemberName] string propertyName = "")
		{
			// string などの参照型が渡されたときも考慮する。
			// Nullable はたとえ null が入っていても、Nullable 自体は値型なので Equals メソッドはいつでも呼び出せるが、参照型はそうではない。
			// Nullable でない値型と null との比較は静的に解析できるので、最適化が行なわれるはず。
			if ((dst == null && src != null) || (dst != null && !dst.Equals(src)))
			{
				dst = src;
				this.NotifyPropertyChanged(propertyName);
				// 通知が発生したときに同時に呼び出す連動処理を Action 経由で渡せるようにすることも検討したが、シンプルさを維持するために却下。
				// 代わりに戻り値をチェックして分岐するようにして欲しい。
				return true;
			}
			else
			{
				return false;
			}
		}
	}


	// Modern UI と従来のデスクトップ UI とで外観を変えるだけにして、内部処理や機能は共通化できるようにするため、
	// コマンドやデータ モデルをビューと分離して、バインディングを使う。
	// http://blog.hiros-dot.net/?p=5742
	// http://yujiro15.net/YKSoftware/MVVM_ICommand.html
	// System.Windows.Input.ICommand は WPF だけでなく、WinRT や Silverlight でも使える模様。
	// https://msdn.microsoft.com/ja-jp/library/hh563947(v=vs.110).aspx
	// 本格的に MVVM でコマンドを扱う場合、Prism や Livet を使ったほうがよい。

	public class MyDelegateCommand : System.Windows.Input.ICommand
	{
		public event Action<object> ExecuteHandler;
		public event Func<object, bool> CanExecuteHandler;

		//public event EventHandler CanExecuteChanged;
		public event EventHandler CanExecuteChanged
		{
			add { System.Windows.Input.CommandManager.RequerySuggested += value; }
			remove { System.Windows.Input.CommandManager.RequerySuggested -= value; }
		}

		/// <summary>
		/// コマンドの実行。
		/// </summary>
		/// <param name="parameter"></param>
		public void Execute(object parameter)
		{
			var handler = this.ExecuteHandler;
			if (handler != null)
			{
				handler(parameter);
			}
		}

		/// <summary>
		/// 実行できるかどうかの判定。
		/// </summary>
		/// <param name="parameter"></param>
		/// <returns></returns>
		public bool CanExecute(object parameter)
		{
			var handler = this.CanExecuteHandler;
			if (handler != null)
			{
				return handler(parameter);
			}
			else
			{
				// デリゲートがひとつも割り当てられていない場合は、既定で有効とする。
				return true;
			}
		}

		/// <summary>
		/// 実行確認メソッド CanExecute の間接起動。
		/// </summary>
		public void RaiseCanExecuteChanged()
		{
			//this.CanExecuteChanged(this, null);
			System.Windows.Input.CommandManager.InvalidateRequerySuggested();
		}
	}

#if false
	public static class MyBindingHelper
	{
		// 元ネタ：http://d.hatena.ne.jp/okazuki/20100106/1262749172

		/// <summary>
		/// イベントを発行する。INotifyPropertyChanged の OnPropertyChanged() 仮想メソッドの個別実装を省力化する。
		/// </summary>
		/// <typeparam name="TResult">プロパティの型。</typeparam>
		/// <param name="eventHandler">イベント ハンドラ。</param>
		/// <param name="propertyName">プロパティ名を表す Expression。() => Name のように指定する。</param>
		public static void RaiseEvent<TResult>(PropertyChangedEventHandler eventHandler,
			System.Linq.Expressions.Expression<Func<TResult>> propertyName)
		{
			// ハンドラに何も登録されていない場合は何もしない。
			if (eventHandler == null)
			{
				return;
			}

			// ラムダ式の Body を取得する。MemberExpression じゃなかったら駄目。
			var memberEx = propertyName.Body as System.Linq.Expressions.MemberExpression;
			if (memberEx == null)
			{
				throw new ArgumentException();
			}

			// () => Name の Name の部分の左側に暗黙的に存在しているオブジェクトを取得する式をゲット。
			var senderExpression = memberEx.Expression as System.Linq.Expressions.ConstantExpression;
			// ConstraintExpression じゃないと駄目。
			if (senderExpression == null)
			{
				throw new ArgumentException();
			}

#if false
			// 式を評価して sender 用のインスタンスを得る。
			var sender = System.Linq.Expressions.Expression.Lambda(senderExpression).Compile().DynamicInvoke();
			// → コレだとだいぶ遅いらしい。
#else
			// 定数なので Value プロパティから sender 用のインスタンスを得る。
			var sender = senderExpression.Value;
#endif

			// 下準備ができたので、イベント発行。
			eventHandler(sender, new PropertyChangedEventArgs(memberEx.Member.Name));
		}
	}
#endif
}
