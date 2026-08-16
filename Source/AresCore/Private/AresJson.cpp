#include "AresCore/AresJson.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace Ares
{

static const FJsonValue GNullValue{};
static const std::string GEmptyString{};

const FJsonValue& FJsonValue::operator[](const std::string& Key) const
{
	if (TypeValue != EJsonType::Object)
	{
		return GNullValue;
	}
	const auto It = Members.find(Key);
	return It == Members.end() ? GNullValue : It->second;
}

const FJsonValue& FJsonValue::operator[](size_t Index) const
{
	if (TypeValue != EJsonType::Array || Index >= Elements.size())
	{
		return GNullValue;
	}
	return Elements[Index];
}

size_t FJsonValue::Num() const
{
	if (TypeValue == EJsonType::Array) return Elements.size();
	if (TypeValue == EJsonType::Object) return Members.size();
	return 0;
}

bool FJsonValue::HasField(const std::string& Key) const
{
	return TypeValue == EJsonType::Object && Members.find(Key) != Members.end();
}

double FJsonValue::AsNumber(double Fallback) const
{
	return TypeValue == EJsonType::Number ? NumberValue : Fallback;
}

bool FJsonValue::AsBool(bool Fallback) const
{
	return TypeValue == EJsonType::Bool ? BoolValue : Fallback;
}

const std::string& FJsonValue::AsString() const
{
	return TypeValue == EJsonType::String ? StringValue : GEmptyString;
}

double FJsonValue::RequireNumber(const std::string& Path, std::string& OutError, double Fallback) const
{
	if (TypeValue != EJsonType::Number)
	{
		if (OutError.empty())
		{
			OutError = "missing or non-numeric field: " + Path;
		}
		return Fallback;
	}
	return NumberValue;
}

/* -------------------------------------------------------------------------- */

class FJsonParser
{
public:
	FJsonParser(const std::string& InText)
		: Text(InText)
	{
	}

	bool Parse(FJsonValue& Out, std::string& OutError)
	{
		SkipWhitespace();
		if (!ParseValue(Out))
		{
			OutError = FormatError();
			return false;
		}
		SkipWhitespace();
		if (Pos != Text.size())
		{
			Error = "trailing content after top-level value";
			OutError = FormatError();
			return false;
		}
		return true;
	}

private:
	const std::string& Text;
	size_t Pos = 0;
	std::string Error;

	std::string FormatError() const
	{
		// Recover line/column so a bad data file points at the offending byte.
		size_t Line = 1;
		size_t Col = 1;
		for (size_t I = 0; I < Pos && I < Text.size(); ++I)
		{
			if (Text[I] == '\n') { ++Line; Col = 1; }
			else { ++Col; }
		}
		char Buffer[64];
		std::snprintf(Buffer, sizeof(Buffer), " (line %zu, column %zu)", Line, Col);
		return (Error.empty() ? std::string("malformed JSON") : Error) + Buffer;
	}

	bool AtEnd() const { return Pos >= Text.size(); }
	char Peek() const { return Pos < Text.size() ? Text[Pos] : '\0'; }

	void SkipWhitespace()
	{
		while (Pos < Text.size())
		{
			const char C = Text[Pos];
			if (C == ' ' || C == '\t' || C == '\n' || C == '\r') { ++Pos; }
			else { break; }
		}
	}

	bool Literal(const char* Word)
	{
		const size_t Len = std::char_traits<char>::length(Word);
		if (Text.compare(Pos, Len, Word) != 0)
		{
			return false;
		}
		Pos += Len;
		return true;
	}

	bool ParseValue(FJsonValue& Out)
	{
		SkipWhitespace();
		if (AtEnd())
		{
			Error = "unexpected end of input";
			return false;
		}

		switch (Peek())
		{
		case '{': return ParseObject(Out);
		case '[': return ParseArray(Out);
		case '"':
			Out.TypeValue = EJsonType::String;
			return ParseString(Out.StringValue);
		case 't':
			if (!Literal("true")) { Error = "expected 'true'"; return false; }
			Out.TypeValue = EJsonType::Bool;
			Out.BoolValue = true;
			return true;
		case 'f':
			if (!Literal("false")) { Error = "expected 'false'"; return false; }
			Out.TypeValue = EJsonType::Bool;
			Out.BoolValue = false;
			return true;
		case 'n':
			if (!Literal("null")) { Error = "expected 'null'"; return false; }
			Out.TypeValue = EJsonType::Null;
			return true;
		default:
			return ParseNumber(Out);
		}
	}

	bool ParseObject(FJsonValue& Out)
	{
		Out.TypeValue = EJsonType::Object;
		++Pos; // '{'
		SkipWhitespace();
		if (Peek() == '}') { ++Pos; return true; }

		for (;;)
		{
			SkipWhitespace();
			if (Peek() != '"') { Error = "expected object key"; return false; }
			std::string Key;
			if (!ParseString(Key)) { return false; }

			SkipWhitespace();
			if (Peek() != ':') { Error = "expected ':' after object key"; return false; }
			++Pos;

			FJsonValue Child;
			if (!ParseValue(Child)) { return false; }
			Out.Members[Key] = std::move(Child);

			SkipWhitespace();
			if (Peek() == ',') { ++Pos; continue; }
			if (Peek() == '}') { ++Pos; return true; }
			Error = "expected ',' or '}' in object";
			return false;
		}
	}

	bool ParseArray(FJsonValue& Out)
	{
		Out.TypeValue = EJsonType::Array;
		++Pos; // '['
		SkipWhitespace();
		if (Peek() == ']') { ++Pos; return true; }

		for (;;)
		{
			FJsonValue Child;
			if (!ParseValue(Child)) { return false; }
			Out.Elements.push_back(std::move(Child));

			SkipWhitespace();
			if (Peek() == ',') { ++Pos; continue; }
			if (Peek() == ']') { ++Pos; return true; }
			Error = "expected ',' or ']' in array";
			return false;
		}
	}

	/** Appends a code point to Out as UTF-8. */
	static void AppendUtf8(std::string& Out, uint32_t Cp)
	{
		if (Cp < 0x80)
		{
			Out.push_back(static_cast<char>(Cp));
		}
		else if (Cp < 0x800)
		{
			Out.push_back(static_cast<char>(0xC0 | (Cp >> 6)));
			Out.push_back(static_cast<char>(0x80 | (Cp & 0x3F)));
		}
		else if (Cp < 0x10000)
		{
			Out.push_back(static_cast<char>(0xE0 | (Cp >> 12)));
			Out.push_back(static_cast<char>(0x80 | ((Cp >> 6) & 0x3F)));
			Out.push_back(static_cast<char>(0x80 | (Cp & 0x3F)));
		}
		else
		{
			Out.push_back(static_cast<char>(0xF0 | (Cp >> 18)));
			Out.push_back(static_cast<char>(0x80 | ((Cp >> 12) & 0x3F)));
			Out.push_back(static_cast<char>(0x80 | ((Cp >> 6) & 0x3F)));
			Out.push_back(static_cast<char>(0x80 | (Cp & 0x3F)));
		}
	}

	bool ParseHex4(uint32_t& Out)
	{
		if (Pos + 4 > Text.size()) { Error = "truncated \\u escape"; return false; }
		Out = 0;
		for (int I = 0; I < 4; ++I)
		{
			const char C = Text[Pos++];
			Out <<= 4;
			if (C >= '0' && C <= '9') Out |= static_cast<uint32_t>(C - '0');
			else if (C >= 'a' && C <= 'f') Out |= static_cast<uint32_t>(C - 'a' + 10);
			else if (C >= 'A' && C <= 'F') Out |= static_cast<uint32_t>(C - 'A' + 10);
			else { Error = "bad hex digit in \\u escape"; return false; }
		}
		return true;
	}

	bool ParseString(std::string& Out)
	{
		++Pos; // opening quote
		Out.clear();
		while (true)
		{
			if (AtEnd()) { Error = "unterminated string"; return false; }
			const char C = Text[Pos++];
			if (C == '"') { return true; }
			if (C != '\\')
			{
				Out.push_back(C);
				continue;
			}
			if (AtEnd()) { Error = "unterminated escape"; return false; }
			const char Esc = Text[Pos++];
			switch (Esc)
			{
			case '"': Out.push_back('"'); break;
			case '\\': Out.push_back('\\'); break;
			case '/': Out.push_back('/'); break;
			case 'b': Out.push_back('\b'); break;
			case 'f': Out.push_back('\f'); break;
			case 'n': Out.push_back('\n'); break;
			case 'r': Out.push_back('\r'); break;
			case 't': Out.push_back('\t'); break;
			case 'u':
			{
				uint32_t Cp = 0;
				if (!ParseHex4(Cp)) { return false; }
				// Recombine a surrogate pair so non-BMP text survives round-trip.
				if (Cp >= 0xD800 && Cp <= 0xDBFF && Pos + 1 < Text.size()
					&& Text[Pos] == '\\' && Text[Pos + 1] == 'u')
				{
					const size_t Save = Pos;
					Pos += 2;
					uint32_t Low = 0;
					if (ParseHex4(Low) && Low >= 0xDC00 && Low <= 0xDFFF)
					{
						Cp = 0x10000 + ((Cp - 0xD800) << 10) + (Low - 0xDC00);
					}
					else
					{
						Pos = Save;
					}
				}
				AppendUtf8(Out, Cp);
				break;
			}
			default:
				Error = "unknown escape sequence";
				return false;
			}
		}
	}

	bool ParseNumber(FJsonValue& Out)
	{
		const size_t Start = Pos;
		if (Peek() == '-' || Peek() == '+') { ++Pos; }
		while (!AtEnd())
		{
			const char C = Peek();
			if ((C >= '0' && C <= '9') || C == '.' || C == 'e' || C == 'E' || C == '+' || C == '-')
			{
				++Pos;
			}
			else
			{
				break;
			}
		}
		if (Pos == Start) { Error = "expected a value"; return false; }

		const std::string Token = Text.substr(Start, Pos - Start);
		char* End = nullptr;
		// strtod is locale-sensitive on the decimal separator; the tests and the
		// data files are ASCII "C" locale and the process never calls setlocale.
		const double Value = std::strtod(Token.c_str(), &End);
		if (End == Token.c_str() || *End != '\0')
		{
			Pos = Start;
			Error = "malformed number";
			return false;
		}
		Out.TypeValue = EJsonType::Number;
		Out.NumberValue = Value;
		return true;
	}
};

bool ParseJson(const std::string& Text, FJsonValue& OutValue, std::string& OutError)
{
	FJsonParser Parser(Text);
	return Parser.Parse(OutValue, OutError);
}

bool ParseJsonFile(const std::string& Path, FJsonValue& OutValue, std::string& OutError)
{
	std::ifstream File(Path, std::ios::binary);
	if (!File)
	{
		OutError = "cannot open file: " + Path;
		return false;
	}
	std::ostringstream Buffer;
	Buffer << File.rdbuf();
	const std::string Text = Buffer.str();
	if (!ParseJson(Text, OutValue, OutError))
	{
		OutError = Path + ": " + OutError;
		return false;
	}
	return true;
}

} // namespace Ares
