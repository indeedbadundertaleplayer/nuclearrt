using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class SetEffect : ActionBase
{
	public override int ObjectType { get; set; } = 2;
	public override int Num { get; set; } = 63;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new StringBuilder();
		result.AppendLine($"for (ObjectIterator it(*{GetSelector(eventBase.ObjectInfo)}); !it.end(); ++it) {{");
		result.AppendLine($"    auto instance = *it;");
		result.AppendLine($"	instance->SetShader(\"{((StringParam)eventBase.Items[0].Loader).Value.Replace(".fx", "")}\");");
		result.AppendLine("}");
		return result.ToString();
	}
}
public class SetEffectParameter : ActionBase
{
	public override int ObjectType { get; set; } = 2;
	public override int Num { get; set; } = 64;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new StringBuilder();
		result.AppendLine($"for (ObjectIterator it(*{GetSelector(eventBase.ObjectInfo)}); !it.end(); ++it) {{");
		result.AppendLine($"    auto instance = *it;");
		result.AppendLine($"	((Active*)instance)->SetShaderParam({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)});");
		result.AppendLine("}");
		return result.ToString();
	}
}
