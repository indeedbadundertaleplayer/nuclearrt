using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;
public class HideMousePointer : ActionBase
{
	public override int Num { get; set; } = 0;
	public override int ObjectType { get; set; } = -6;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"Application::Instance().GetBackend()->{(eventBase.Num == 0 ? "HideMouseCursor" : "ShowMouseCursor")}();";
	}
}
public class ShowMousePointer : HideMousePointer
{
	public override int Num { get; set; } = 1;
}
