#pragma once
#include "Dom/JsonValue.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonReader.h"
#include "Policies/CondensedJsonPrintPolicy.h"
namespace OdinUtility
{
constexpr uint8 EODIN_ERROR_OFFSET = 128;

typedef TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>> FCondensedJsonStringWriterFactory;
typedef TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>        FCondensedJsonStringWriter;
typedef TJsonReaderFactory<TCHAR>                                   FCondensedJsonStringReaderFactory;
typedef TJsonReader<TCHAR>                                          FCondensedJsonStringReader;
typedef TArray<TSharedPtr<FJsonValue>>::TConstIterator              FCondensedJsonArrayIterator;
} // namespace OdinUtility
