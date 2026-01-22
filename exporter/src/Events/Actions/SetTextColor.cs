using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;
using CTFAK.MMFParser.MFA.Loaders;
using System.Text;
public class SetTextColor : ActionBase
{
	public override int ObjectType { get; set; } = 3;
	public override int Num { get; set; } = 56;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new StringBuilder();
		string val = "";
		if (eventBase.Items[0].Loader is Colour colorParameter) val = $"Application::Instance().GetColor({colorParameter.Value.R}, {colorParameter.Value.G}, {colorParameter.Value.B})";
		else if (eventBase.Items[0].Loader is ExpressionParameter expressionParameter)
		{
			val = $"{ExpressionConverter.ConvertExpression(expressionParameter, eventBase)}";
		}
		result.AppendLine($"for (ObjectIterator it(*{GetSelector(eventBase.ObjectInfo)}); !it.end(); ++it) {{");
		result.AppendLine($"  auto instance = *it;");
		result.AppendLine($"  ((StringObject*)instance)->SetColor({val});");
		result.AppendLine("}");
		return result.ToString();
	}
}
public class SetTextColor2 : SetTextColor
{
	public override int Num { get; set; } = 83;
}
