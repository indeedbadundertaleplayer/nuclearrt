using CTFAK.Memory;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Expressions;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class PerspectiveExporter : ExtensionExporter
{
	public override string ObjectIdentifier => "tksp";
	public override string ExtensionName => "Perspective";
	public override string CppClassName => "PerspectiveExtension";
	public override string ExportExtension(byte[] extensionData)
	{
		ByteReader reader = new ByteReader(extensionData);
		reader.ReadInt16();
		reader.ReadInt16();
		short Width = reader.ReadInt16();
		short Height = reader.ReadInt16();
		short Flags = reader.ReadInt16();
		short Effect = reader.ReadInt16();
		return CreateExtension($"{Width}, {Height}, {Effect}, {Flags}");
	}
}
