// AresCore — minimal dependency-free JSON reader.
//
// Why this exists: brief §9 says "if a number appears in data/*.json, read it
// from the DataTable. Never duplicate a balance constant in C++." AresCore must
// also unit-test headless (§3.1), so it cannot lean on UE's Json module to do
// that reading in tests. Rather than take a third-party dependency — which
// brief §2 forbids without approval — this is a compact reader covering exactly
// the subset of JSON that data/*.json uses.
//
// Under UE this is still the loader AresCore uses; AresEditor's importers
// (M2) convert the same files into DataTables for cooked builds.

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Ares
{

enum class EJsonType : uint8_t
{
	Null,
	Bool,
	Number,
	String,
	Array,
	Object,
};

class FJsonValue
{
public:
	FJsonValue() = default;

	EJsonType Type() const { return TypeValue; }
	bool IsNull() const { return TypeValue == EJsonType::Null; }
	bool IsNumber() const { return TypeValue == EJsonType::Number; }
	bool IsObject() const { return TypeValue == EJsonType::Object; }
	bool IsArray() const { return TypeValue == EJsonType::Array; }

	/** Object member lookup. Returns a static null value when absent — never throws. */
	const FJsonValue& operator[](const std::string& Key) const;
	/** Array element lookup. Returns a static null value when out of range. */
	const FJsonValue& operator[](size_t Index) const;

	size_t Num() const;
	bool HasField(const std::string& Key) const;

	double AsNumber(double Fallback = 0.0) const;
	bool AsBool(bool Fallback = false) const;
	const std::string& AsString() const;

	const std::vector<FJsonValue>& AsArray() const { return Elements; }
	const std::map<std::string, FJsonValue>& AsObject() const { return Members; }

	/**
	 * Reads a required number. Sets OutError and returns Fallback when the field
	 * is missing or not a number, so a malformed data file fails loudly at load
	 * instead of silently balancing the game to zero.
	 */
	double RequireNumber(const std::string& Path, std::string& OutError, double Fallback = 0.0) const;

private:
	friend class FJsonParser;

	EJsonType TypeValue = EJsonType::Null;
	double NumberValue = 0.0;
	bool BoolValue = false;
	std::string StringValue;
	std::vector<FJsonValue> Elements;
	std::map<std::string, FJsonValue> Members;
};

/** Parses JSON text. On failure returns false and fills OutError with a line/column. */
bool ParseJson(const std::string& Text, FJsonValue& OutValue, std::string& OutError);

/** Reads a file from disk and parses it. Convenience for headless tests and tools. */
bool ParseJsonFile(const std::string& Path, FJsonValue& OutValue, std::string& OutError);

} // namespace Ares
