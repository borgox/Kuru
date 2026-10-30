#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Kuru::Common::DumpSchema {

constexpr std::uint32_t CurrentVersion = 3;

struct GlobalAddresses {
    std::uintptr_t gNames = 0;
    std::uintptr_t gObjects = 0;
    std::uintptr_t gMalloc = 0;
    std::uintptr_t gWorld = 0;
    std::uintptr_t gEngine = 0;
};

struct PropertyRecord {
    std::string name;
    std::string kind;
    std::string cppType;
    std::uint64_t flags = 0;
    std::int32_t offset = 0;
    std::int32_t elementSize = 0;
    std::int32_t arrayDim = 0;
    std::string inner;
    // A dynamic array has one inner property, including its referenced type and size.
    std::vector<PropertyRecord> innerProperties;
    std::string outer;
    std::string propertyClass;
    std::string metaClass;
    std::string structName;
    std::string enumName;
    std::string interfaceClass;
    std::string key;
    std::string value;
    std::string bitMask;
};

struct FunctionRecord {
    std::string fullName;
    std::uint64_t flags = 0;
    std::uint16_t parmsSize = 0;
    std::uint8_t numParms = 0;
    std::uint16_t returnValueOffset = 0xFFFF;
    std::vector<PropertyRecord> params;
};

struct ObjectRecord {
    std::uintptr_t address = 0;
    std::string name;
    std::string fullName;
    std::string packageName;
    std::string className;
    std::string outerName;
    std::string superName;
    std::int32_t propertySize = 0;
    std::int32_t minAlignment = 0;
    std::vector<PropertyRecord> properties;
    std::vector<FunctionRecord> functions;
    std::vector<std::string> enumNames;
};

struct DumpDocument {
    std::uint32_t version = CurrentVersion;
    std::string game;
    std::string buildLabel;
    GlobalAddresses globals;
    std::vector<std::string> names;
    std::vector<ObjectRecord> objects;
};

} // namespace Kuru::Common::DumpSchema
