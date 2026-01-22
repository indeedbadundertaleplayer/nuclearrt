using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;
public class EndOfFrame : ConditionBase
{
	public override int ObjectType { get; set; } = -3;
	public override int Num { get; set; } = -2;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"{ifStatement} (Application::Instance().GetCurrentState() == GameState::EndFrame || Application::Instance().GetCurrentState() == GameState::EndApplication)) goto {nextLabel};"; // EndOfApplication also triggers EndOfFrame
	}
}
