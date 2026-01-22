using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;
using System.Text;

public class SetAnimationSpeed : ActionBase
{
	public override int Num { get; set; } = 19;
	public override int ObjectType { get; set; } = 2;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new StringBuilder();
		result.AppendLine($"for (ObjectIterator it(*{GetSelector(eventBase.ObjectInfo)}); !it.end(); ++it) {{");
		result.AppendLine("		auto instance = *it;");
		result.AppendLine($"	((Active*)instance)->animations.SetAnimationSpeed({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
		result.AppendLine("}");
		return result.ToString();
	}
}
