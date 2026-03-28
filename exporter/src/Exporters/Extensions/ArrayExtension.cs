using CTFAK.Memory;
using CTFAK.CCN.Chunks.Frame;
using System.Text;
using CTFAK.Utils;
using CTFAK.MMFParser.EXE.Loaders.Events.Expressions;
public class ArrayExtension : ExtensionExporter
{
	public override string ObjectIdentifier => "0RRA";
	public override string ExtensionName => "KcArray";
	public override string CppClassName => "ArrayExtension";
	public override string ExportExtension(byte[] extensionData)
	{
		ByteReader reader = new ByteReader(extensionData);
		short Xdimension = reader.ReadInt16();
		short Ydimension = reader.ReadInt16();
		short Zdimension = reader.ReadInt16();
		short Type = reader.ReadInt16();
		short Flags = reader.ReadInt16();
		return CreateExtension($"{Xdimension}, {Ydimension}, {Zdimension}, {Type}, {Flags}");
	}
	public override string ExportAction(EventBase eventBase, int actionNum, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, bool isGlobal = false)
	{
		StringBuilder result = new StringBuilder();
		switch (actionNum)
		{
			default:
				result.AppendLine($"// Array action {actionNum} not implemented");
				break;
		}
		return result.ToString();
	}
	public override string ExportCondition(EventBase eventBase, int conditionNum, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (", bool isGlobal = false)
	{
		StringBuilder result = new StringBuilder();
		switch (conditionNum)
		{
			default:
				result.AppendLine($"// Array condition {conditionNum} not implemented");
				result.AppendLine($"goto {nextLabel};");
				break;
		}
		return result.ToString();
	}
	public override string ExportExpression(Expression expression, EventBase eventBase = null)
	{
		string result = string.Empty;
		switch (expression.Num)
		{
			default:
				result = $"0 /* Array expression {expression.Num} not implemented */";
				break;
		}
		return result;
	}
} 
