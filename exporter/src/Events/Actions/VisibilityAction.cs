using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class MakeInvisibleAction : ActionBase
{
	public override int ObjectType { get; set; } = 2;
	public override int Num { get; set; } = 26;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new StringBuilder();

		result.AppendLine($"for (ObjectIterator it(*{GetSelector(eventBase.ObjectInfo)}); !it.end(); ++it) {{");
		result.AppendLine($"    auto instance = *it;");
		result.AppendLine($"    (({ExpressionConverter.GetObjectClassName(eventBase.ObjectInfo, IsGlobal)}*)instance)->Visible = {(eventBase.Num == 26 ? false : true).ToString().ToLower()};");
		result.AppendLine($"    (({ExpressionConverter.GetObjectClassName(eventBase.ObjectInfo, IsGlobal)}*)instance)->IsFlashing = false;");
		result.AppendLine("}");

		return result.ToString();
	}
}

public class ReappearAction : MakeInvisibleAction
{
	public override int Num { get; set; } = 27;
}

public class CounterMakeInvisibleAction : MakeInvisibleAction
{
	public override int ObjectType { get; set; } = 7;
}

public class CounterReappearAction : ReappearAction
{
	public override int ObjectType { get; set; } = 7;
}

public class StringMakeInvisibleAction : MakeInvisibleAction
{
	public override int ObjectType { get; set; } = 3;
}

public class StringReappearAction : ReappearAction
{
	public override int ObjectType { get; set; } = 3;
}
public class FlashAction : ActionBase
{
	public override int ObjectType {get; set; } = -2;
	public override int Num { get; set; } = 28;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		string val;
		if (eventBase.Items[0].Loader is Time time) val = time.Timer.ToString();
		else if (eventBase.Items[0].Loader is ExpressionParameter expressionParameter) val = ExpressionConverter.ConvertExpression(expressionParameter, eventBase);
		else val = "0";
		StringBuilder result = new StringBuilder();
		result.AppendLine($"for (ObjectIterator it(*{GetSelector(eventBase.ObjectInfo)}); !it.end(); ++it) {{");
		result.AppendLine("		auto instance = *it;");
		result.AppendLine($"    (({ExpressionConverter.GetObjectClassName(eventBase.ObjectInfo, IsGlobal)}*)instance)->IsFlashing = true;");
		result.AppendLine($"	if (GameTimer.CheckEvent({parameters["eventIndex"]}, {val}, TimerEventType::Every)) {{");
		result.AppendLine($"			if ((({ExpressionConverter.GetObjectClassName(eventBase.ObjectInfo, IsGlobal)}*)instance)->IsFlashing)");
		result.AppendLine($"				(({ExpressionConverter.GetObjectClassName(eventBase.ObjectInfo, IsGlobal)}*)instance)->Visible = !(({ExpressionConverter.GetObjectClassName(eventBase.ObjectInfo, IsGlobal)}*)instance)->Visible");
		result.AppendLine("		}");
		result.AppendLine("}");
		return result.ToString();
	}
}
public class FlashString : FlashAction
{
	public override int ObjectType { get; set; } = 3;
}
public class FlashCounter : FlashAction
{
	public override int ObjectType { get; set; } = 7;
}
