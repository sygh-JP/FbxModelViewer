using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;
using System.Xml;

namespace MyMiscHelpers
{
	public static class MyGenericsHelper
	{
		/// <summary>
		/// IDisposable を安全に Dispose する汎用メソッド。
		/// </summary>
		/// <typeparam name="Type">IDisposable</typeparam>
		/// <param name="obj"></param>
		public static void SafeDispose<Type>(ref Type obj)
			where Type : IDisposable
		{
			if (obj != null)
			{
				obj.Dispose();
				obj = default(Type); // null 非許容型への対応。
			}
		}

		/// <summary>
		/// ジェネリクスによる汎用 Clamp メソッド。
		/// </summary>
		/// <typeparam name="Type"></typeparam>
		/// <param name="x"></param>
		/// <param name="min"></param>
		/// <param name="max"></param>
		/// <returns></returns>
		public static Type Clamp<Type>(Type x, Type min, Type max)
			where Type : IComparable
		{
			if (x.CompareTo(min) < 0)
				return min;
			else if (x.CompareTo(max) > 0)
				return max;
			else
				return x;
		}

		/// <summary>
		/// ジェネリクスによる汎用 Swap メソッド。
		/// </summary>
		/// <typeparam name="Type"></typeparam>
		/// <param name="a"></param>
		/// <param name="b"></param>
		public static void Swap<Type>(ref Type a, ref Type b)
		{
			Type c = b;
			b = a;
			a = c;
		}

		/// <summary>
		/// Nullable もしくは参照型に null が格納されている場合に明示的なリテラル文字列に変換する。
		/// Nullable に null が格納されている場合、通常は ToString() によって空文字列が返却される。
		/// 参照型に null が格納されている場合、ToString() を呼び出すと NullReferenceException になる。
		/// </summary>
		/// <typeparam name="T"></typeparam>
		/// <param name="obj"></param>
		/// <returns></returns>
		public static string ConvertToLiteralNullIfNull<T>(T obj)
		{
			return (obj != null) ? obj.ToString() : "null";
			// C# 6.0 以降であれば、obj?.ToString() ?? "null" と書いてもよい。ただし厳密には完全互換でない。
			// C# 5.0 以前の古いコンパイラもサポートしたいので、あえて古い書き方にとどめる。
		}

		public static T AssignNewIfNull<T>(T obj)
			where T : new()
		{
			if (obj == null)
			{
				return new T();
			}
			else
			{
				return obj;
			}
		}

		public static void AssignNewIfNull<T>(ref T obj)
			where T : new()
		{
			if (obj == null)
			{
				obj = new T();
			}
		}

		public static string GetMemberName<T>(System.Linq.Expressions.Expression<Func<T>> e)
		{
			var memberExp = (System.Linq.Expressions.MemberExpression)e.Body;
			return memberExp.Member.Name;
		}


		public static T GetValueFrom<T>(SerializationInfo info, string name)
		{
			return (T)info.GetValue(name, typeof(T));
		}

		public static T GetPropertyValueFrom<T>(SerializationInfo info, System.Linq.Expressions.Expression<Func<T>> propertyNameExpression)
		{
			return GetValueFrom<T>(info, GetMemberName(propertyNameExpression));
		}

		public static void AddValueTo<T>(SerializationInfo info, string name, T inValue)
		{
			info.AddValue(name, inValue);
		}

		public static void AddPropertyValueTo<T>(SerializationInfo info, System.Linq.Expressions.Expression<Func<T>> propertyNameExpression, T inValue)
		{
			AddValueTo<T>(info, GetMemberName(propertyNameExpression), inValue);
		}

		// Dictionary には IDictionary を受け取るコピーコンストラクタがあるが、テーブルの値が参照型だとシャローコピーになる。
		// 下記は T[] や List<T> を値とするようなテーブルを、1段だけディープコピーするヘルパーメソッド。末尾までディープコピーしたければ再帰が必要。

		public static Dictionary<TKey, TListValue[]> CreateDeepCopyL1<TKey, TListValue>(Dictionary<TKey, TListValue[]> srcData)
		{
			return srcData.ToDictionary(item => item.Key, item => item.Value.ToArray());
		}

		public static Dictionary<TKey, List<TListValue>> CreateDeepCopyL1<TKey, TListValue>(Dictionary<TKey, List<TListValue>> srcData)
		{
			return srcData.ToDictionary(item => item.Key, item => item.Value.ToList());
		}

		public static TEnum GetUnionOfAllKnownFlagsInEnum<TEnum>()
			where TEnum : struct
		{
			// C# では型制約に enum が使えない。[Flags] つまり FlagsAttribute が指定されているかどうかも制約できない。
			// 少々邪道だが、dynamic を使って回避。ただし実行時に動的に解決することになるので、パフォーマンスに劣る。
			// 起動時に1回だけ実行する、などの運用に限定すること。
			// F# だと静的に書けそう。
			dynamic outVal = default(TEnum);
			// TEnum が enum でない場合は Enum.GetValues() で例外がスローされる。
			foreach (TEnum x in Enum.GetValues(typeof(TEnum)))
			{
				// FlagsAttribute が指定されていることが前提。
				outVal |= x;
			}
			return outVal;
		}
	}


	/// <summary>
	/// シリアル化できる、System.Collections.Generic.KeyValuePair に代わる構造体。
	/// </summary>
	/// <typeparam name="TKey">Key の型。</typeparam>
	/// <typeparam name="TValue">Value の型。</typeparam>
	[Serializable]
	public struct KeyAndValue<TKey, TValue>
	{
		public TKey Key;
		public TValue Value;

		public KeyAndValue(KeyValuePair<TKey, TValue> pair)
		{
			Key = pair.Key;
			Value = pair.Value;
		}
	}

	// Dictionary/Set は、XmlSerializer では直接 XML にシリアライズできない。
	// DataContractSerializer を使うか、List/Array を経由する。

	public class MyObjectSerializationHelper
	{
		/// <summary>
		/// Dictionary を KeyAndValue の List に変換する。
		/// </summary>
		/// <typeparam name="TKey">Dictionary の Key の型。</typeparam>
		/// <typeparam name="TValue">Dictionary の Value の型。</typeparam>
		/// <param name="src">変換する Dictionary。</param>
		/// <returns>変換された KeyAndValue の List。</returns>
		public static List<KeyAndValue<TKey, TValue>> ConvertDictionaryToList<TKey, TValue>(Dictionary<TKey, TValue> src)
		{
			var dst = new List<KeyAndValue<TKey, TValue>>(src.Count());
			foreach (var pair in src)
			{
				dst.Add(new KeyAndValue<TKey, TValue>(pair));
			}
			return dst;
		}

		/// <summary>
		/// KeyAndValue の List を Dictionary に変換する。
		/// </summary>
		/// <typeparam name="TKey">KeyAndValue の Key の型。</typeparam>
		/// <typeparam name="TValue">KeyAndValue の Value の型。</typeparam>
		/// <param name="src">変換する KeyAndValue の List。</param>
		/// <returns>変換された Dictionary。</returns>
		public static Dictionary<TKey, TValue> ConvertListToDictionary<TKey, TValue>(List<KeyAndValue<TKey, TValue>> src)
		{
			var dst = new Dictionary<TKey, TValue>(src.Count());
			foreach (var pair in src)
			{
				dst.Add(pair.Key, pair.Value);
			}
			return dst;
		}

		#region DataContract

		public static void SerializeToXmlFileByDataContract<T>(string filename, T srcObject)
		{
			var serializer = new DataContractSerializer(typeof(T));
			var settings = new XmlWriterSettings();
			settings.Encoding = new System.Text.UTF8Encoding(true);
			settings.Indent = true;
			using (var stream = File.Open(filename, FileMode.Create, FileAccess.Write, FileShare.Read))
			{
				using (var xw = XmlWriter.Create(stream, settings))
				{
					serializer.WriteObject(xw, srcObject);
				}
			}
		}

		public static T DeserializeFromXmlFileByDataContract<T>(string filename)
		{
			var serializer = new DataContractSerializer(typeof(T));
			using (var stream = File.Open(filename, FileMode.Open, FileAccess.Read, FileShare.Read))
			{
				using (var xr = XmlReader.Create(stream))
				{
					return (T)serializer.ReadObject(xr);
				}
			}
		}

		public static T DeserializeFromXmlFileByDataContractIfExists<T>(string filename)
			where T : new()
		{
			if (File.Exists(filename))
			{
				return DeserializeFromXmlFileByDataContract<T>(filename);
			}
			else
			{
				return new T();
			}
		}

		#endregion

		// IXmlSerializable にも対応できるように、ISerializable には限定しない。

		public static void SerializeToBinaryFile<T>(string filename, T srcObject)
			//where T : ISerializable
		{
			using (var stream = File.Open(filename, FileMode.Create, FileAccess.Write, FileShare.Read))
			{
				var formatter = new System.Runtime.Serialization.Formatters.Binary.BinaryFormatter();
				formatter.Serialize(stream, srcObject);
			}
		}

		public static T DeserializeFromBinaryFile<T>(string filename)
			//where T : ISerializable
		{
			using (var stream = File.Open(filename, FileMode.Open, FileAccess.Read, FileShare.Read))
			{
				var formatter = new System.Runtime.Serialization.Formatters.Binary.BinaryFormatter();
				return (T)formatter.Deserialize(stream);
			}
		}

		public static T DeserializeFromBinaryFileIfExists<T>(string filename)
			where T : new()
		{
			if (File.Exists(filename))
			{
				return DeserializeFromBinaryFile<T>(filename);
			}
			else
			{
				return new T();
			}
		}

		public static void SerializeToXmlFile<T>(string filename, T srcObject)
			//where T : ISerializable
		{
			using (var stream = File.Open(filename, FileMode.Create, FileAccess.Write, FileShare.Read))
			{
				using (var sw = new StreamWriter(stream, new System.Text.UTF8Encoding(true)))
				{
					var serializer = new System.Xml.Serialization.XmlSerializer(typeof(T));
					serializer.Serialize(sw, srcObject);
				}
			}
		}

		public static T DeserializeFromXmlFile<T>(string filename)
			//where T : ISerializable
		{
			using (var stream = File.Open(filename, FileMode.Open, FileAccess.Read, FileShare.Read))
			{
				using (var sr = new StreamReader(stream, new System.Text.UTF8Encoding(true)))
				{
					var serializer = new System.Xml.Serialization.XmlSerializer(typeof(T));
					return (T)serializer.Deserialize(sr);
				}
			}
		}

		public static T DeserializeFromXmlFileIfExists<T>(string filename)
			where T : new()
		{
			if (File.Exists(filename))
			{
				return DeserializeFromXmlFile<T>(filename);
			}
			else
			{
				return new T();
			}
		}
	}
}
