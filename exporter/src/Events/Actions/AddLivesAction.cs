using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class AddLivesAction : ActionBase
{
	public override int ObjectType { get; set; } = -7;
	public override int Num { get; set; } = 5;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"Application::Instance().GetAppData()->AddLives({eventBase.ObjectInfo}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});";
	}
}
