using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using CTFAK.Memory;
using CTFAK.MMFParser.EXE.Loaders.Events.Expressions;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;
using CTFAK.Utils;

namespace CTFAK.CCN.Chunks.Frame
{
	public class Events : ChunkLoader
	{
		public readonly string Header = "ER>>";
		public readonly string EventCount = "ERes";
		public readonly string EventgroupData = "ERev";
		public readonly string EventOptions = "ERop";
		public readonly string End = "<<ER";

		public int MaxObjects;
		public int MaxObjectInfo;
		public int NumberOfPlayers;
		public int OptionFlags;
		public List<Quailifer> QualifiersList = new List<Quailifer>();
		public List<int> NumberOfConditions = new List<int>();
		public List<EventGroup> Items = new List<EventGroup>();

		public static int IdentifierCounter;

		public override void Read(ByteReader reader)
		{
			IdentifierCounter = 0;
			// if (Settings.GameType == GameType.OnePointFive) return;
			while (true)
			{
				var identifier = reader.ReadAscii(4);
				if (identifier == Header)
				{
					MaxObjects = reader.ReadInt16();
					MaxObjectInfo = reader.ReadInt16();
					NumberOfPlayers = reader.ReadInt16();
					for (int i = 0; i < 17; i++)
					{
						NumberOfConditions.Add(reader.ReadInt16());
					}

					var qualifierCount = reader.ReadInt16(); //should be 0, so i dont care
					for (int i = 0; i < qualifierCount; i++)
					{
						var newQualifier = new Quailifer();
						newQualifier.Read(reader);
						QualifiersList.Add(newQualifier);
					}
				}
				else if (identifier == EventCount)
				{
					var size = reader.ReadInt32();
				}
				else if (identifier == EventgroupData)
				{
					var size = reader.ReadInt32();

					var endPosition = reader.Tell() + size;
					while (true)
					{
						var eg = new EventGroup();
						eg.Read(reader);
						Items.Add(eg);

						if (reader.Tell() >= endPosition) break;
					}

				}
				else if (identifier == EventOptions)
				{
					OptionFlags = reader.ReadInt32();
				}
				else if (identifier == End) break;
			}
		}

	}

	public class Quailifer : ChunkLoader
	{
		public int ObjectInfo;
		public int Type;
		public int Qualifier;
		List<int> _objects = new List<int>();

		public override void Read(ByteReader reader)
		{
			ObjectInfo = reader.ReadUInt16();
			Type = reader.ReadInt16();
			Qualifier = ObjectInfo & 0b11111111111;
		}
	}

	public class EventGroup : ChunkLoader
	{
		public ushort Flags;
		public int IsRestricted;
		public int RestrictCpt;
		public int Identifier;
		public int Undo;
		public List<Condition> Conditions = new List<Condition>();
		public List<Action> Actions = new List<Action>();
		public int Size;
		public byte NumberOfConditions;
		public byte NumberOfActions;
		public bool isMFA = false;

		public override void Read(ByteReader reader)
		{
			var currentPosition = reader.Tell();
			Size = reader.ReadInt16();
			NumberOfConditions = reader.ReadByte();
			NumberOfActions = reader.ReadByte();
			Flags = reader.ReadUInt16();

			IsRestricted = reader.ReadInt16(); //For MFA
			RestrictCpt = reader.ReadInt16();
			Identifier = reader.ReadInt16();
			Undo = reader.ReadInt16();

			for (int i = 0; i < NumberOfConditions; i++)
			{
				var item = new Condition();
				item.Read(reader);
				item.Identifier += Events.IdentifierCounter;
				Fixer.FixConditions(ref item);
				if (item.Num == -27 && item.ObjectType == -1 ||
						item.Num == -43 && item.ObjectType == -1)
				{
					//breaks mfa reading
				}
				else if (item.Num == -25 || item.Num == -41)
				{
					if (item.Items.Count > 0)
					{
						bool isNormal;
						foreach (var param in item.Items)
						{
							if (param.Loader is MultipleVariables multivar)
							{
								int cnt = 0;
								int mask = 1;
								while (true)
								{
									if (mask == 0) break;
									if ((mask & multivar.flagMasks) == 0)
									{
										mask <<= 1;
										cnt++;
										continue;
									}
									var newCondition = new Condition();
									newCondition.DefType = item.DefType;
									newCondition.Identifier = item.Identifier + cnt;
									newCondition.ObjectInfo = item.ObjectInfo;
									newCondition.Flags = item.Flags;
									newCondition.OtherFlags = item.OtherFlags;
									newCondition.ObjectType = item.ObjectType;

									//Alterable Flags
									newCondition.Num = ((mask & multivar.flagValues) == 0) ? -24 : -25;
									var exp = new ExpressionParameter() { Comparsion = 0 };
									exp.Items.Add(new Expression()
									{ Loader = new LongExp() { Value = cnt }, ObjectType = -1 });
									newCondition.Items.Add(new Parameter() { Code = 22, Loader = exp });
									Conditions.Add(newCondition);
									mask <<= 1;
									cnt++;
									Events.IdentifierCounter++;
								}

								for (int j = 0; j < multivar.values.Length; j++)
								{
									var val = multivar.values[j];
									var newCondition = new Condition();
									newCondition.DefType = item.DefType;
									newCondition.Identifier = item.Identifier + j;
									newCondition.ObjectInfo = item.ObjectInfo;
									newCondition.Flags = item.Flags;
									newCondition.OtherFlags = item.OtherFlags;
									newCondition.ObjectType = item.ObjectType;

									//Alterable Values
									newCondition.Num = -27;
									var newParam = new AlterableValue();
									newParam.Value = (short)val.index;
									newCondition.Items.Add(new Parameter() { Code = 50, Loader = newParam });
									var exp = new ExpressionParameter() { Comparsion = (short)val.op };
									if (val.isDouble)
										exp.Items.Add(new Expression() { Loader = new DoubleExp() { Value = (double)val.value }, ObjectType = -1, Num = 23 });
									else
										exp.Items.Add(new Expression() { Loader = new LongExp() { Value = (int)val.value }, ObjectType = -1 });
									newCondition.Items.Add(new Parameter() { Code = 23, Loader = exp });
									Conditions.Add(newCondition);
									Events.IdentifierCounter++;
								}
							}
							else
								Conditions.Add(item);
						}
					}
					else
						Conditions.Add(item);
				}
				else
					Conditions.Add(item);
			}

			for (int i = 0; i < NumberOfActions; i++)
			{
				var item = new Action();
				item.Read(reader);
				Fixer.FixActions(ref item);
				if (item.Num == 43 && item.ObjectType == -1) { }
				else if (item.Num == 2 && item.Items.Count == 2)
				{
					var xAct = new Action();
					xAct.DefType = item.DefType;
					xAct.ObjectInfo = item.ObjectInfo;
					xAct.Flags = item.Flags;
					xAct.OtherFlags = item.OtherFlags;
					xAct.ObjectType = item.ObjectType;
					xAct.Num = 2;
					xAct.Items.Add(item.Items[0]);
					var yAct = new Action();
					yAct.DefType = item.DefType;
					yAct.ObjectInfo = item.ObjectInfo;
					yAct.Flags = item.Flags;
					yAct.OtherFlags = item.OtherFlags;
					yAct.ObjectType = item.ObjectType;
					yAct.Num = 3;
					yAct.Items.Add(item.Items[1]);
					Actions.Add(xAct);
					Actions.Add(yAct);
				}
				else Actions.Add(item);
			}

			reader.Seek(currentPosition + (Size * -1));
		}
	}

	public static class Fixer
	{
		public static void FixConditions(ref Condition cond)
		{
			var num = cond.Num;
			//Alterable Values:
			if ((num == -42 || num == -43) && cond.ObjectType != -1) num = -27;
			//Global Values
			if (cond.ObjectType == -1)
				if (num == -28 || num == -29 || num == -30 || num == -31 || num == -32 || num == -33)
					num = -8;
			cond.Num = num;
		}
		public static void FixActions(ref Action act)
		{
			var num = act.Num;
			var type = act.ObjectType;
			if (type == -1)
			{
				if (num == 27 || num == 28 || num == 29 || num == 30)
					num = 3;
				if (num == 35 || num == 32 || num == 33 || num == 34)
					num = 4;
				if (num == 31 || num == 36 || num == 37 || num == 38)
					num = 5;
			}
			act.Num = num;
		}
	}

	public class EventBase : ChunkLoader
	{
		public int Flags;
		public int OtherFlags;
		public int DefType;
		public int ObjectType;
		public int Num;
		public int ObjectInfo;
		public int ObjectInfoList;

		public byte NumberOfParameters;
		public List<Parameter> Items = new List<Parameter>();

		public override void Read(ByteReader reader)
		{
			throw new NotImplementedException();
		}

		public bool IsOfType(ACBase acBase)
		{
			return ObjectType == acBase.ObjectType && Num == acBase.Num;
		}
	}

	public class Condition : EventBase
	{
		public int Identifier;

		public override void Read(ByteReader reader)
		{
			var currentPosition = reader.Tell();
			var size = reader.ReadUInt16();

			ObjectType = reader.ReadInt16();
			Num = reader.ReadInt16();
			ObjectInfo = reader.ReadUInt16();
			ObjectInfoList = reader.ReadInt16();
			Flags = reader.ReadSByte();
			OtherFlags = reader.ReadSByte();
			NumberOfParameters = reader.ReadByte();
			DefType = reader.ReadByte();
			Identifier = reader.ReadInt16();
			if (CTFAKCore.parameters.Contains("-noevnt")) return;
			else
			{
				for (int i = 0; i < NumberOfParameters; i++)
				{
					var item = new Parameter();
					item.Read(reader);
					Items.Add(item);
				}
			}

			//Logger.Log(this);
			//Console.ReadKey();
		}
		public override string ToString()
		{
			//return Preprocessor.ProcessCondition(this);
			//return $"Condition {(Constants.ObjectType)ObjectType}=={Names.ConditionNames[ObjectType][Num]}{(Items.Count > 0 ? "-"+Items[0].ToString() : " ")}";
			return $"Condition {(Constants.ObjectType)ObjectType}=={Num}{(Items.Count > 0 ? "-" + Items[0].Loader.ToString() : " ")} Params: {Items.Count}";
		}
	}

	public class Action : EventBase
	{
		public override void Read(ByteReader reader)
		{
			var old = false;
			var currentPosition = reader.Tell();
			var size = reader.ReadUInt16();
			ObjectType = old ? reader.ReadSByte() : reader.ReadInt16();
			Num = old ? reader.ReadSByte() : reader.ReadInt16();
			if (ObjectType >= 2 && Num >= 48)
				if (old) Num += 32;
			ObjectInfo = reader.ReadUInt16();
			ObjectInfoList = reader.ReadInt16();
			Flags = reader.ReadSByte();
			OtherFlags = reader.ReadSByte();
			NumberOfParameters = reader.ReadByte();
			DefType = reader.ReadByte();
			for (int i = 0; i < NumberOfParameters; i++)
			{
				var item = new Parameter();
				item.Read(reader);
				Items.Add(item);
			}
			//Logger.Log(this);
		}
		public override string ToString()
		{
			string str = $"Action {ObjectType} : {Num} : {Flags} : {OtherFlags} : {DefType} : {ObjectInfo} : {ObjectInfoList} : Params: {NumberOfParameters}";
			foreach (var param in Items)
				str += " : " + param.Loader.ToString();
			return str;
		}
	}

	public class Parameter : ChunkLoader
	{
		public int Code;
		public ChunkLoader Loader;

		public override void Read(ByteReader reader)
		{
			var currentPosition = reader.Tell();
			var size = reader.ReadInt16();
			Code = reader.ReadInt16();

			Loader = LoadParameter(Code, reader);
			if (Loader != null) Loader.Read(reader);
			else
			{
				Logger.LogWarning("Loader is null: " + Code);
				return;
			}

			reader.Seek(currentPosition + size);
		}

		public object Value
		{
			get
			{
				if (Loader != null)
				{
					if (Loader.GetType().GetField("value") != null)
					{
						return Loader.GetType().GetField("value").GetValue(Loader);
					}
					else
					{
						return null;
					}
				}
				else return null;
			}
		}
		public static ChunkLoader LoadParameter(int code, ByteReader reader)
		{
			ChunkLoader item = null;
			if (code == 1)
				item = new ParamObject();
			if (code == 2 || code == 42)
				item = new Time();
			if (code == 3 || code == 4 || code == 10 || code == 11 || code == 12 || code == 17 ||
					code == 26 || code == 31 || code == 37 || code == 43 || code == 57 || code == 58 || code == 60 ||
					code == 61)
				item = new Short();
			if (code == 5 || code == 25 || code == 29 || code == 34 || code == 48 || code == 56)
				item = new IntParam();
			if (code == 6 || code == 7 || code == 35 || code == 36)
				item = new Sample();
			if (code == 9 || code == 21)
				item = new Create();
			if (code == 13)
				item = new Every();
			if (code == 14 || code == 44)
				item = new KeyParameter();
			if (code == 15 || code == 22 || code == 23 || code == 27 || code == 28 || code == 45 ||
					code == 46 || code == 52 || code == 53 || code == 54 || code == 59 || code == 62)
				item = new ExpressionParameter();
			if (code == 16)
				item = new Position();
			if (code == 18)
				item = new Shoot();
			if (code == 19)
				item = new Zone();
			if (code == 24)
				item = new Colour();
			if (code == 40)
				item = new Filename();
			if (code == 50)
				item = new AlterableValue();
			if (code == 32)
				item = new Click();
			if (code == 33)
				item = new Program();
			if (code == 55)
				item = new MMFParser.EXE.Loaders.Events.Parameters.Extension();
			if (code == 38)
				item = new Group();
			if (code == 39)
				item = new GroupPointer();
			if (code == 49)
				item = new GlobalValue();
			if (code == 41 || code == 64)
				item = new StringParam();
			if (code == 47 || code == 51)
				item = new TwoShorts();
			if (code == 67)
				item = new IntParam();
			if (code == 68)
				item = new MultipleVariables();
			if (code == 69)
				item = new ChildEvent();
			if (code == 70)
				item = new IntParam();

			return item;
		}
	}
}
