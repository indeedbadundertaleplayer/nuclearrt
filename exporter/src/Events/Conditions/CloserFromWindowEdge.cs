using System;
using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;
public class CloserFromWindowEdge : ConditionBase
{
	public override int ObjectType { get; set; } = 2;
	public override int Num { get; set; } = -22;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new StringBuilder();
		result.AppendLine($"for (ObjectIterator it(*{GetSelector(eventBase.ObjectInfo)}); !it.end(); ++it) {{");
		result.AppendLine($"	bool isCollide = false;");
		result.AppendLine($"	auto instance = *it;");
		result.AppendLine($"	float pixelGiven = {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)};"); // Floats CAN be given to the expression.... so....
		result.AppendLine($"	for (int i = 0; i < static_cast<int>(pixelGiven); ++i) {{");
		result.AppendLine($"		if (IsColliding(instance, i, instance->Y) || IsColliding(instance, Application::Instance().GetAppData()->GetWindowWidth() - i, instance->Y) || IsColliding(instance, instance->X, i) || IsColliding(instance, instance->X, Application::Instance().GetAppData()->GetWindowHeight() - i)) isCollide = true;");
		result.AppendLine($"		else continue;");
		result.AppendLine($"	}}");
		result.AppendLine($"	{ifStatement} (isCollide)) goto {nextLabel};");
		result.AppendLine($"}}");
		return result.ToString();
	}
}
