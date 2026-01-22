using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;
using System;
using System.Text;
public class SetXScale : ActionBase
{
	public override int ObjectType { get; set; } = 2;
	public override int Num { get; set; } = 86;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new StringBuilder();

		result.AppendLine($"for (ObjectIterator it(*{GetSelector(eventBase.ObjectInfo)}); !it.end(); ++it) {{");
		result.AppendLine($"    auto instance = *it;");
		result.AppendLine($"    ((Active*)instance)->scale{(eventBase.Num == 86 ? "X" : "Y")} = {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)};");
		result.AppendLine("}");

		return result.ToString();
	}
}
public class SetYScale : SetXScale
{
	public override int Num { get; set; } = 87;
}
