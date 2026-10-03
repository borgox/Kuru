#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <Core/ReflectedTypeRegistry.hpp>
#include <Kuru/Common/DumpSchema.hpp>

namespace {

const std::unordered_set<std::string_view> CppKeywords = {
    "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool", "break", "case",
    "catch", "char", "char8_t", "char16_t", "char32_t", "class", "compl", "concept", "const", "consteval",
    "constexpr", "constinit", "const_cast", "continue", "co_await", "co_return", "co_yield", "decltype",
    "default", "delete", "do", "double", "dynamic_cast", "else", "enum", "explicit", "export", "extern",
    "false", "float", "for", "friend", "goto", "if", "inline", "int", "long", "mutable", "namespace", "new",
    "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected", "public",
    "register", "reinterpret_cast", "requires", "return", "short", "signed", "sizeof", "static",
    "static_assert", "static_cast", "struct", "switch", "template", "this", "thread_local", "throw", "true",
    "try", "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual", "void", "volatile",
    "wchar_t", "while", "xor", "xor_eq",
};

struct PropertyInfo {
    std::string Name;
    std::string Kind;
    std::string StructName;
    std::string PropertyClass;
    std::string InnerKind;
    std::string EnumName;
    std::string BitMask;
    std::uint64_t Flags = 0;
    std::uint32_t Offset = 0;
    std::uint32_t ElementSize = 0;
    std::uint32_t ArrayDim = 1;
};

struct FunctionInfo {
    std::string Name;
    std::string FullName;
    std::uint32_t ParamsSize = 0;
    std::uint32_t ReturnValueOffset = 0xFFFF;
    std::vector<PropertyInfo> Params;
};

struct TypeInfo {
    std::string Name;
    std::string FullName;
    std::string PackageName;
    std::string ClassName;
    std::string SuperName;
    std::uint32_t PropertySize = 0;
    std::uint32_t MinAlignment = 1;
    std::vector<PropertyInfo> Properties;
    std::vector<FunctionInfo> Functions;
    std::vector<std::string> EnumNames;
};

struct DumpModel {
    std::vector<TypeInfo> Types;
    std::unordered_map<std::string, std::uint32_t> SizeByFullName;
    std::unordered_map<std::string, std::string> SuperByFullName;
    std::unordered_set<std::string> ClassNames;
    std::unordered_set<std::string> StructNames;
    std::unordered_set<std::string> EnumTypeNames;
    std::unordered_map<std::string, std::string> CppNamesByFullName;
    std::size_t NameCollisions = 0;
    std::vector<std::pair<std::string, std::string>> NameCollisionMappings;
};

[[nodiscard]] bool IsActorDescendant(std::string fullName, const DumpModel& model);

[[nodiscard]] bool IsManualType(std::string_view fullName) {
    return Kuru::SDK::Metadata::IsBaseReflectedType(fullName);
}

[[nodiscard]] std::string Escape(std::string_view value) {
    std::string out;
    out.reserve(value.size());
    for (const char ch : value) {
        if (ch == '\\' || ch == '"') {
            out.push_back('\\');
        }
        out.push_back(ch);
    }
    return out;
}

[[nodiscard]] std::string SanitizeIdentifier(std::string_view value) {
    std::string out;
    out.reserve(value.size() + 1);
    for (const unsigned char ch : value) {
        out.push_back(std::isalnum(ch) || ch == '_' ? static_cast<char>(ch) : '_');
    }
    if (out.empty()) {
        out = "Unnamed";
    }
    if (std::isdigit(static_cast<unsigned char>(out.front()))) {
        out.insert(out.begin(), '_');
    }
    while (out.size() > 1 && out[0] == '_' && out[1] == '_') {
        out.erase(out.begin());
    }
    if (CppKeywords.contains(out)) {
        out += '_';
    }
    return out;
}

[[nodiscard]] std::string LeafName(std::string_view fullName) {
    const std::size_t pos = fullName.find_last_of('.');
    return std::string(pos == std::string_view::npos ? fullName : fullName.substr(pos + 1));
}

[[nodiscard]] DumpModel BuildModel(const Kuru::Common::DumpSchema::DumpDocument& document) {
    DumpModel model;

    const auto convertProperty = [](const Kuru::Common::DumpSchema::PropertyRecord& source) {
        PropertyInfo property;
        property.Name = source.name;
        property.Kind = source.kind;
        property.StructName = source.structName;
        property.PropertyClass = source.propertyClass;
        property.InnerKind = source.inner;
        property.EnumName = source.enumName;
        property.BitMask = source.bitMask;
        property.Flags = source.flags;
        property.Offset = static_cast<std::uint32_t>(std::max(source.offset, 0));
        property.ElementSize = static_cast<std::uint32_t>(std::max(source.elementSize, 0));
        property.ArrayDim = static_cast<std::uint32_t>(std::max(source.arrayDim, 1));
        return property;
    };

    for (const Kuru::Common::DumpSchema::ObjectRecord& source : document.objects) {
        if (source.className != "Class" && source.className != "ScriptStruct" && source.className != "Enum") {
            continue;
        }

        TypeInfo type;
        type.Name = source.name.empty() ? LeafName(source.fullName) : source.name;
        type.FullName = source.fullName;
        type.PackageName = source.packageName.empty() ? "Unknown" : source.packageName;
        type.ClassName = source.className;
        type.SuperName = source.superName.empty() ? "None" : source.superName;
        type.PropertySize = static_cast<std::uint32_t>(std::max(source.propertySize, 0));
        type.MinAlignment = static_cast<std::uint32_t>(std::max(source.minAlignment, 1));
        for (const auto& property : source.properties) type.Properties.push_back(convertProperty(property));
        type.EnumNames = source.enumNames;
        for (const auto& sourceFunction : source.functions) {
            FunctionInfo function;
            function.FullName = sourceFunction.fullName;
            function.Name = LeafName(function.FullName);
            function.ParamsSize = sourceFunction.parmsSize;
            function.ReturnValueOffset = sourceFunction.returnValueOffset;
            for (const auto& parameter : sourceFunction.params) function.Params.push_back(convertProperty(parameter));
            type.Functions.push_back(std::move(function));
        }

        if (type.PropertySize != 0) {
            model.SizeByFullName[type.FullName] = type.PropertySize;
        }
        if (type.ClassName == "Class") {
            model.SuperByFullName[type.FullName] = type.SuperName;
            model.ClassNames.insert(type.FullName);
        } else if (type.ClassName == "ScriptStruct") {
            model.StructNames.insert(type.FullName);
        } else if (type.ClassName == "Enum") {
            model.EnumTypeNames.insert(type.FullName);
        }
        model.Types.push_back(std::move(type));
    }

    std::vector<const TypeInfo*> nameOrder;
    for (const TypeInfo& type : model.Types) {
        if (!IsManualType(type.FullName)) nameOrder.push_back(&type);
    }
    std::sort(nameOrder.begin(), nameOrder.end(), [](const TypeInfo* lhs, const TypeInfo* rhs) {
        return lhs->FullName < rhs->FullName;
    });
    std::unordered_map<std::string, std::size_t> useCount;
    for (const TypeInfo* type : nameOrder) {
        const std::string leaf = SanitizeIdentifier(LeafName(type->FullName));
        const std::string prefix = type->ClassName == "Class" ? (IsActorDescendant(type->FullName, model) ? "A" : "U")
            : type->ClassName == "Enum" ? "E" : "F";
        std::string name = prefix + leaf;
        if (useCount.contains(name)) {
            ++model.NameCollisions;
            name = prefix + SanitizeIdentifier(type->PackageName) + "_" + leaf;
            for (std::size_t suffix = 2; useCount.contains(name); ++suffix) name = prefix + SanitizeIdentifier(type->PackageName) + "_" + leaf + "_" + std::to_string(suffix);
            model.NameCollisionMappings.emplace_back(type->FullName, name);
        }
        useCount.emplace(name, 1);
        model.CppNamesByFullName.emplace(type->FullName, std::move(name));
    }
    return model;
}

[[nodiscard]] bool IsActorDescendant(std::string fullName, const DumpModel& model) {
    for (int i = 0; i < 256 && fullName != "None" && !fullName.empty(); ++i) {
        if (fullName == "Engine.Actor") {
            return true;
        }
        const auto it = model.SuperByFullName.find(fullName);
        if (it == model.SuperByFullName.end()) {
            return false;
        }
        fullName = it->second;
    }
    return false;
}

[[nodiscard]] std::string CppTypeName(std::string_view fullName, const DumpModel& model) {
    if (const std::string_view manualCppName = Kuru::SDK::Metadata::FindBaseReflectedCppName(fullName);
        !manualCppName.empty()) {
        return std::string(manualCppName);
    }
    if (const auto found = model.CppNamesByFullName.find(std::string(fullName)); found != model.CppNamesByFullName.end()) {
        return found->second;
    }
    const std::string leaf = SanitizeIdentifier(LeafName(fullName));
    if (model.ClassNames.contains(std::string(fullName))) {
        return std::string(IsActorDescendant(std::string(fullName), model) ? "A" : "U") + leaf;
    }
    return "F" + leaf;
}

[[nodiscard]] std::string QualifiedReflectedTypeName(std::string_view fullName, const DumpModel& model) {
    if (Kuru::SDK::Metadata::IsBaseReflectedType(fullName)) {
        return std::string(Kuru::SDK::Metadata::FindBaseReflectedCppName(fullName));
    }
    return "Kuru::RL::" + CppTypeName(fullName, model);
}

[[nodiscard]] std::string FieldType(const PropertyInfo& prop, const DumpModel& model) {
    if (prop.Kind == "IntProperty") {
        return "std::int32_t";
    }
    if (prop.Kind == "FloatProperty") {
        return "float";
    }
    if (prop.Kind == "BoolProperty") {
        return prop.ElementSize == 1 ? "bool" : "std::uint32_t";
    }
    if (prop.Kind == "ByteProperty") {
        if (!prop.EnumName.empty() && prop.EnumName != "None" && model.EnumTypeNames.contains(prop.EnumName)) {
            return QualifiedReflectedTypeName(prop.EnumName, model);
        }
        return "std::uint8_t";
    }
    if (prop.Kind == "QWordProperty") {
        return "std::uint64_t";
    }
    if (prop.Kind == "SQWordProperty") {
        return "std::int64_t";
    }
    if (prop.Kind == "NameProperty") {
        return "Kuru::SDK::FName";
    }
    if (prop.Kind == "StrProperty") {
        return "Kuru::SDK::FString";
    }
    if (prop.Kind == "ObjectProperty" || prop.Kind == "ComponentProperty") {
        return prop.PropertyClass.empty() ? "Kuru::SDK::UObject*" : QualifiedReflectedTypeName(prop.PropertyClass, model) + "*";
    }
    if (prop.Kind == "ClassProperty") {
        return prop.PropertyClass.empty() ? "Kuru::SDK::UClass*" : QualifiedReflectedTypeName(prop.PropertyClass, model) + "*";
    }
    if (prop.Kind == "DelegateProperty") {
        return "Kuru::SDK::FMultiScriptDelegate";
    }
    if (prop.Kind == "ArrayProperty") {
        return "Kuru::SDK::TArray<std::uint8_t>";
    }
    if (prop.Kind == "MapProperty" && prop.ElementSize == sizeof(void*) * 10) {
        return "Kuru::SDK::TMap<std::uint8_t, std::uint8_t>";
    }
    if (prop.Kind == "StructProperty") {
        if (!prop.StructName.empty() && (model.SizeByFullName.contains(prop.StructName) ||
                                         Kuru::SDK::Metadata::IsBaseReflectedType(prop.StructName))) {
            return QualifiedReflectedTypeName(prop.StructName, model);
        }
        if (prop.StructName == "Core.Object.Vector") {
            return "Kuru::SDK::FVector";
        }
        if (prop.StructName == "Core.Object.Vector2D") {
            return "Kuru::SDK::FVector2D";
        }
        if (prop.StructName == "Core.Object.Vector4" || prop.StructName == "Core.Object.Plane") {
            return "Kuru::SDK::FVector4";
        }
        if (prop.StructName == "Core.Object.Rotator") {
            return "Kuru::SDK::FRotator";
        }
        if (prop.StructName == "Core.Object.Color") {
            return "Kuru::SDK::FColor";
        }
        if (prop.StructName == "Core.Object.LinearColor") {
            return "Kuru::SDK::FLinearColor";
        }
        if (prop.StructName == "Core.Object.Guid") {
            return "Kuru::SDK::FGuid";
        }
        if (prop.StructName == "Core.Object.Pointer") {
            return "void*";
        }
    }
    return {};
}

[[nodiscard]] std::uint32_t FieldStorageSize(const PropertyInfo& prop, std::string_view type) {
    if (type.empty()) {
        return prop.ElementSize * prop.ArrayDim;
    }
    return prop.ElementSize * prop.ArrayDim;
}

[[nodiscard]] std::uint32_t DeclaredFieldSize(const PropertyInfo& prop, const DumpModel& model) {
    if (prop.Kind == "StructProperty") {
        if (const auto it = model.SizeByFullName.find(prop.StructName); it != model.SizeByFullName.end()) return it->second;
    }
    return prop.ElementSize;
}

void AppendPad(std::string& text, std::uint32_t offset, std::uint32_t size) {
    if (size == 0) {
        return;
    }
    text += "    std::uint8_t Pad_";
    std::ostringstream hex;
    hex << std::uppercase << std::hex;
    hex.width(4);
    hex.fill('0');
    hex << offset;
    text += hex.str();
    text += "[0x";
    hex.str("");
    hex.clear();
    hex << std::uppercase << std::hex << size;
    text += hex.str();
    text += "];\n";
}

void AppendProperty(std::string& text, const PropertyInfo& prop, const DumpModel& model, std::unordered_set<std::string>& names) {
    std::string name = SanitizeIdentifier(prop.Name);
    while (!names.insert(name).second) {
        name += "_";
    }
    const std::string type = FieldType(prop, model);
    if (prop.Kind == "BoolProperty" && prop.ElementSize == 4 && !prop.BitMask.empty()) {
        text += "    std::uint32_t ";
        text += name;
        text += " : 1;";
    } else if (type.empty()) {
        text += "    std::uint8_t ";
        text += name;
        text += "[0x";
        std::ostringstream size;
        size << std::uppercase << std::hex << (prop.ElementSize * prop.ArrayDim);
        text += size.str();
        text += "];";
    } else {
        text += "    ";
        text += type;
        text += ' ';
        text += name;
        if (prop.ArrayDim > 1) {
            text += '[';
            text += std::to_string(prop.ArrayDim);
            text += ']';
        }
        text += ';';
    }
    text += " // 0x";
    std::ostringstream offset;
    offset << std::uppercase << std::hex << prop.Offset;
    text += offset.str();
    text += ' ';
    text += prop.Kind;
    if (!prop.StructName.empty()) {
        text += ' ';
        text += prop.StructName;
    }
    text += "\n";
    const std::uint32_t declaredSize = DeclaredFieldSize(prop, model) * prop.ArrayDim;
    const std::uint32_t reflectedSlotSize = prop.ElementSize * prop.ArrayDim;
    if (declaredSize < reflectedSlotSize) {
        AppendPad(text, prop.Offset + declaredSize, reflectedSlotSize - declaredSize);
    }
}

void AppendInheritedPropertyAccessor(std::string& text, const PropertyInfo& prop, const DumpModel& model,
                                     std::unordered_set<std::string>& names) {
    std::string name = SanitizeIdentifier(prop.Name);
    while (!names.insert("accessor_" + name).second) name += "_";
    std::ostringstream offset;
    offset << std::uppercase << std::hex << prop.Offset;

    if (prop.Kind == "BoolProperty" && prop.ElementSize == 4 && !prop.BitMask.empty()) {
        text += "    [[nodiscard]] bool Get" + name + "() const {\n";
        text += "        constexpr std::uint32_t Mask = " + prop.BitMask + ";\n";
        text += "        const auto* storage = reinterpret_cast<const std::uint32_t*>(reinterpret_cast<const std::uint8_t*>(this) + 0x" + offset.str() + ");\n";
        text += "        return (*storage & Mask) != 0;\n";
        text += "    }\n";
        text += "    void Set" + name + "(bool value) {\n";
        text += "        constexpr std::uint32_t Mask = " + prop.BitMask + ";\n";
        text += "        auto* storage = reinterpret_cast<std::uint32_t*>(reinterpret_cast<std::uint8_t*>(this) + 0x" + offset.str() + ");\n";
        text += "        *storage = value ? (*storage | Mask) : (*storage & ~Mask);\n";
        text += "    }\n";
        return;
    }

    const std::string type = FieldType(prop, model);
    if (type.empty() || prop.ArrayDim != 1) {
        text += "    // Reflected inherited field " + name + " has no safe typed accessor.\n";
        return;
    }
    text += "    [[nodiscard]] " + type + "& Get" + name + "() {\n";
    text += "        return *reinterpret_cast<" + type + "*>(reinterpret_cast<std::uint8_t*>(this) + 0x" + offset.str() + ");\n";
    text += "    }\n";
    text += "    [[nodiscard]] const " + type + "& Get" + name + "() const {\n";
    text += "        return *reinterpret_cast<const " + type + "*>(reinterpret_cast<const std::uint8_t*>(this) + 0x" + offset.str() + ");\n";
    text += "    }\n";
}

constexpr std::uint64_t CPF_Parm = 0x80;
constexpr std::uint64_t CPF_OutParm = 0x100;
constexpr std::uint64_t CPF_ReturnParm = 0x400;

[[nodiscard]] bool IsReturnParam(const PropertyInfo& property) {
    return (property.Flags & (CPF_Parm | CPF_ReturnParm)) == (CPF_Parm | CPF_ReturnParm);
}
[[nodiscard]] bool IsOutParam(const PropertyInfo& property) {
    return (property.Flags & (CPF_Parm | CPF_OutParm)) == (CPF_Parm | CPF_OutParm) && !IsReturnParam(property);
}
[[nodiscard]] bool IsInputParam(const PropertyInfo& property) { return (property.Flags & CPF_Parm) != 0 && !IsReturnParam(property); }

[[nodiscard]] std::string ParameterType(const PropertyInfo& property, const DumpModel& model) {
    const std::string elementType = property.Kind == "BoolProperty" ? "bool" : FieldType(property, model);
    const std::string resolvedType = elementType.empty()
        ? "std::array<std::uint8_t, " + std::to_string(property.ElementSize) + ">"
        : elementType;
    return property.ArrayDim > 1
        ? "std::array<" + resolvedType + ", " + std::to_string(property.ArrayDim) + ">"
        : resolvedType;
}

void AppendParamsStruct(std::string& text, const FunctionInfo& fn, const DumpModel& model, std::string_view paramsName,
                        std::unordered_map<const PropertyInfo*, std::string>& parameterNames) {
    text += "    struct ";
    text += paramsName;
    text += " {\n";
    std::vector<const PropertyInfo*> properties;
    for (const PropertyInfo& property : fn.Params) {
        if ((property.Flags & CPF_Parm) != 0) properties.push_back(&property);
    }
    std::sort(properties.begin(), properties.end(), [](const PropertyInfo* lhs, const PropertyInfo* rhs) {
        return lhs->Offset == rhs->Offset ? lhs->Name < rhs->Name : lhs->Offset < rhs->Offset;
    });
    std::unordered_set<std::string> names;
    std::uint32_t cursor = 0;
    for (const PropertyInfo* property : properties) {
        const std::uint32_t storageSize = property->ElementSize * property->ArrayDim;
        if (property->Offset + storageSize > fn.ParamsSize) continue;
        if (property->Offset < cursor) continue;
        AppendPad(text, cursor, property->Offset - cursor);
        std::string name = SanitizeIdentifier(property->Name);
        while (!names.insert(name).second) name += "_";
        parameterNames.emplace(property, name);
        text += "        ";
        text += ParameterType(*property, model);
        text += ' ';
        text += name;
        text += "; // 0x";
        std::ostringstream offset;
        offset << std::uppercase << std::hex << property->Offset;
        text += offset.str();
        text += "\n";
        if (property->Kind == "BoolProperty" && property->ElementSize > sizeof(bool)) {
            AppendPad(text, property->Offset + static_cast<std::uint32_t>(sizeof(bool)),
                      property->ElementSize - static_cast<std::uint32_t>(sizeof(bool)));
        } else {
            const std::uint32_t declaredSize = DeclaredFieldSize(*property, model) * property->ArrayDim;
            if (declaredSize < storageSize) {
                AppendPad(text, property->Offset + declaredSize, storageSize - declaredSize);
            }
        }
        cursor = property->Offset + property->ElementSize * property->ArrayDim;
    }
    AppendPad(text, cursor, fn.ParamsSize > cursor ? fn.ParamsSize - cursor : 0);
    text += "    };\n";
    if (fn.ParamsSize != 0) {
        text += "    static_assert(sizeof(";
        text += paramsName;
        text += ") == 0x";
        std::ostringstream size;
        size << std::uppercase << std::hex << fn.ParamsSize;
        text += size.str();
        text += ");\n";
    }
}

void AppendFunctionWrapper(std::string& text, const FunctionInfo& fn, const DumpModel& model,
                           std::unordered_set<std::string>& names) {
    std::string name = SanitizeIdentifier(fn.Name);
    while (!names.insert("fn_" + name).second) {
        name += "_";
    }

    std::unordered_map<const PropertyInfo*, std::string> parameterNames;
    const std::string paramsName = name + "_Params";
    AppendParamsStruct(text, fn, model, paramsName, parameterNames);

    const PropertyInfo* returnProperty = nullptr;
    for (const PropertyInfo& property : fn.Params) if (IsReturnParam(property)) { returnProperty = &property; break; }
    text += "    ";
    text += returnProperty ? ParameterType(*returnProperty, model) : "void";
    text += ' ';
    text += name;
    text += '(';
    bool comma = false;
    for (const PropertyInfo& property : fn.Params) {
        if (!IsInputParam(property) || !parameterNames.contains(&property)) continue;
        if (comma) text += ", ";
        const std::string type = ParameterType(property, model);
        text += type;
        if (IsOutParam(property)) text += "&";
        text += ' ';
        text += parameterNames.find(&property)->second;
        comma = true;
    }
    text += ") {\n";
    if (fn.ParamsSize != 0) {
        text += "        "; text += paramsName; text += " params{};\n";
        for (const PropertyInfo& property : fn.Params) {
            if (!IsInputParam(property) || !parameterNames.contains(&property)) continue;
            const std::string& parameter = parameterNames.find(&property)->second;
            text += "        params." + parameter + " = " + parameter + ";\n";
        }
    }
    text += "        static Kuru::SDK::UFunction* function = Kuru::SDK::UObject::FindFunction(\"";
    text += Escape(fn.FullName);
    text += "\");\n";
        text += "        this->ProcessEventChecked(function, ";
    text += fn.ParamsSize == 0 ? "nullptr" : "&params";
    text += ");\n";
    for (const PropertyInfo& property : fn.Params) {
        if (!IsOutParam(property) || !parameterNames.contains(&property)) continue;
        const std::string& parameter = parameterNames.find(&property)->second;
        text += "        " + parameter + " = params." + parameter + ";\n";
    }
    if (returnProperty && parameterNames.contains(returnProperty)) {
        text += "        return params." + parameterNames.find(returnProperty)->second + ";\n";
    } else if (returnProperty) {
        text += "        return {}; // Reflected return storage overlaps a non-parameter local.\n";
    }
    text += "    }\n";
}

void AppendBaseExtensionWrapper(std::string& text, const TypeInfo& type, const FunctionInfo& fn, const DumpModel& model,
                                std::unordered_set<std::string>& names) {
    std::string name = SanitizeIdentifier(fn.Name);
    while (!names.insert("fn_" + name).second) {
        name += "_";
    }

    std::unordered_map<const PropertyInfo*, std::string> parameterNames;
    const std::string paramsName = name + "_Params";
    AppendParamsStruct(text, fn, model, paramsName, parameterNames);

    text += "inline void ";
    text += name;
    text += '(';
    text += std::string(Kuru::SDK::Metadata::FindBaseReflectedCppName(type.FullName));
    text += "* self";
    if (fn.ParamsSize != 0) {
        text += ", ";
        text += paramsName;
        text += "& params";
    }
    text += ") {\n";
    text += "    if (!self) {\n";
    text += "        return;\n";
    text += "    }\n";
    text += "    static Kuru::SDK::UFunction* function = Kuru::SDK::UObject::FindFunction(\"";
    text += Escape(fn.FullName);
    text += "\");\n";
    text += "    self->ProcessEventChecked(function, ";
    text += fn.ParamsSize == 0 ? "nullptr" : "&params";
    text += ");\n";
    text += "}\n\n";
}

[[nodiscard]] std::string MakeManualTypesHeader() {
    std::string text;
    text += "#pragma once\n\n";
    text += "#include <string_view>\n\n";
    text += "namespace Kuru::Generated {\n\n";
    text += "inline constexpr std::string_view ManualBaseTypes[] = {\n";
    for (const auto& type : Kuru::SDK::Metadata::BaseReflectedTypes) {
        text += "    \"";
        text += type.FullName;
        text += "\",\n";
    }
    text += "};\n\n";
    text += "[[nodiscard]] constexpr bool IsManualBaseType(std::string_view fullName) {\n";
    text += "    for (std::string_view type : ManualBaseTypes) {\n";
    text += "        if (type == fullName) {\n";
    text += "            return true;\n";
    text += "        }\n";
    text += "    }\n";
    text += "    return false;\n";
    text += "}\n\n";
    text += "} // namespace Kuru::Generated\n";
    return text;
}

[[nodiscard]] std::string MakeReflectedTypesHeader(const DumpModel& model) {
    std::string text;
    text += "#pragma once\n\n";
    text += "#include <string_view>\n\n";
    text += "namespace Kuru::Generated {\n\n";
    text += "inline constexpr std::string_view ReflectedTypes[] = {\n";
    for (const TypeInfo& type : model.Types) {
        if (IsManualType(type.FullName)) {
            continue;
        }
        text += "    \"";
        text += Escape(type.FullName);
        text += "\",\n";
    }
    text += "};\n\n";
    text += "} // namespace Kuru::Generated\n";
    return text;
}

[[nodiscard]] std::string MakeForwardDeclarations(const DumpModel& model) {
    std::string text;
    text += "#pragma once\n\n";
    text += "#include <KuruSDKBase.hpp>\n\n";
    text += "namespace Kuru::RL {\n\n";
    for (const TypeInfo& type : model.Types) {
        if (IsManualType(type.FullName) || type.ClassName == "Enum") {
            continue;
        }
        text += type.ClassName == "Class" ? "class " : "struct ";
        text += CppTypeName(type.FullName, model);
        text += ";\n";
    }
    text += "\n} // namespace Kuru::RL\n";
    return text;
}

[[nodiscard]] std::string MakeEnumDefinition(const TypeInfo& type, const DumpModel& model) {
    std::string text;
    text += "enum class ";
    text += CppTypeName(type.FullName, model);
    text += " : std::uint8_t {\n";
    std::unordered_set<std::string> names;
    for (std::size_t i = 0; i < type.EnumNames.size(); ++i) {
        std::string name = SanitizeIdentifier(type.EnumNames[i]);
        while (!names.insert(name).second) {
            name += "_";
        }
        text += "    ";
        text += name;
        text += " = ";
        text += std::to_string(i);
        text += ",\n";
    }
    text += "};\n\n";
    return text;
}

/*
 * Parse a BoolProperty bit mask, RL stores this as a 64 bit field
 *
 */
[[nodiscard]] std::uint64_t ParseBitMask(const std::string& mask) {
    if (mask.empty()) return 0;
    try {
        return std::stoull(mask, nullptr, 0);
    } catch (const std::exception&) {
        return 0;
    }
}

[[nodiscard]] std::string MakeStructOrClassDefinition(const TypeInfo& type, const DumpModel& model) {
    std::string text;
    const bool isClass = type.ClassName == "Class";
    const std::string cppName = CppTypeName(type.FullName, model);
    std::string super = "Kuru::SDK::UObject";
    if (!type.SuperName.empty() && type.SuperName != "None") {
        super = CppTypeName(type.SuperName, model);
    }

    text += isClass ? "class " : "struct ";
    text += cppName;
    if (isClass || (!type.SuperName.empty() && type.SuperName != "None")) {
        text += " : public ";
        text += super;
    }
    text += " {\n";
    text += "public:\n";

    std::vector<PropertyInfo> props = type.Properties;
    std::sort(props.begin(), props.end(), [](const PropertyInfo& lhs, const PropertyInfo& rhs) {
        if (lhs.Offset != rhs.Offset) {
            return lhs.Offset < rhs.Offset;
        }
        return lhs.Name < rhs.Name;
    });

    std::uint32_t cursor = 0;
    if (!type.SuperName.empty() && type.SuperName != "None") {
        if (const auto it = model.SizeByFullName.find(type.SuperName); it != model.SizeByFullName.end()) {
            cursor = it->second;
        }
    }

    std::unordered_set<std::string> memberNames;
    for (std::size_t propertyIndex = 0; propertyIndex < props.size(); ++propertyIndex) {
        const PropertyInfo& prop = props[propertyIndex];
        const std::uint32_t storageSize = FieldStorageSize(prop, FieldType(prop, model));
        if (storageSize == 0) {
            continue;
        }
        if (prop.Kind == "BoolProperty" && prop.ElementSize == 4 && !prop.BitMask.empty()) {
            std::size_t groupEnd = propertyIndex + 1;
            while (groupEnd < props.size() && props[groupEnd].Offset == prop.Offset &&
                   props[groupEnd].Kind == "BoolProperty" && props[groupEnd].ElementSize == 4 &&
                   !props[groupEnd].BitMask.empty()) {
                ++groupEnd;
            }
            if (prop.Offset < cursor) {
                for (std::size_t index = propertyIndex; index < groupEnd; ++index) {
                    AppendInheritedPropertyAccessor(text, props[index], model, memberNames);
                }
                propertyIndex = groupEnd - 1;
                continue;
            }
            AppendPad(text, cursor, prop.Offset - cursor);
            std::vector<const PropertyInfo*> bools;
            for (std::size_t index = propertyIndex; index < groupEnd; ++index) bools.push_back(&props[index]);
            std::sort(bools.begin(), bools.end(), [](const PropertyInfo* lhs, const PropertyInfo* rhs) {
                return ParseBitMask(lhs->BitMask) < ParseBitMask(rhs->BitMask);
            });
            /* Emit anonymous fill bits for reserved/unused bits so each bool lands on its true
             bit_mask position. Plain sequential ":1" packing misaligns non-contiguous masks
             (e.g. Core.FeatureSystem: Prime=0x2, Matchmaking=0x8 with 0x1/0x4 reserved) and
             would make writing one bool flip another.
            */
            unsigned nextBit = 0;
            for (const PropertyInfo* bit : bools) {
                const std::uint64_t mask = ParseBitMask(bit->BitMask);
                unsigned bitIndex = 0;
                while (bitIndex < 32 && !((mask >> bitIndex) & 1ULL)) ++bitIndex;
                if (bitIndex > nextBit) {
                    text += "    std::uint32_t : " + std::to_string(bitIndex - nextBit) + ";\n";
                }
                AppendProperty(text, *bit, model, memberNames);
                nextBit = bitIndex + 1;
            }
            text += "    std::uint32_t : 0;\n";
            cursor = prop.Offset + prop.ElementSize;
            propertyIndex = groupEnd - 1;
            continue;
        }
        if (prop.Offset < cursor) {
            AppendInheritedPropertyAccessor(text, prop, model, memberNames);
            continue;
        }
        AppendPad(text, cursor, prop.Offset - cursor);
        AppendProperty(text, prop, model, memberNames);
        cursor = prop.Offset + storageSize;
    }
    if (type.PropertySize > cursor) {
        AppendPad(text, cursor, type.PropertySize - cursor);
    }
    if (props.empty() && type.Functions.empty() && type.PropertySize <= cursor) {
        text += "    // No reflected fields or wrappers in current dump.\n";
    }

    if (isClass) {
        text += "\n";
        text += "    static Kuru::SDK::UClass* StaticClass() {\n";
        text += "        return Kuru::SDK::UObject::FindClass(\"";
        text += Escape(type.FullName);
        text += "\");\n";
        text += "    }\n";
        text += "    static ";
        text += cppName;
        text += "* GetDefaultObject() {\n";
        text += "        Kuru::SDK::UClass* klass = StaticClass();\n";
        text += "        return klass ? static_cast<";
        text += cppName;
        text += "*>(klass->GetDefaultObject()) : nullptr;\n";
        text += "    }\n";
    }

    std::unordered_set<std::string> functionNames;
    for (const FunctionInfo& fn : type.Functions) {
        text += "\n";
        AppendFunctionWrapper(text, fn, model, functionNames);
    }

    text += "};\n";
    if (type.PropertySize != 0) {
        text += "static_assert(sizeof(";
        text += cppName;
        text += ") == 0x";
        std::ostringstream size;
        size << std::uppercase << std::hex << type.PropertySize;
        text += size.str();
        text += ");\n";
    }
    text += "\n";
    return text;
}

[[nodiscard]] std::string MakePackageHeader(std::string_view package, const std::vector<const TypeInfo*>& types,
                                            const DumpModel& model) {
    std::string text;
    text += "#pragma once\n\n";
    text += "#include <Generated/ForwardDeclarations.hpp>\n\n";
    text += "#include <array>\n#include <cstdint>\n\n";
    text += "namespace Kuru::RL {\n\n";
    text += "// Reflected package: ";
    text += package;
    text += "\n\n";
    text += "#pragma pack(push, 1)\n\n";
    for (const TypeInfo* type : types) {
        if (type->ClassName == "Enum") {
            text += MakeEnumDefinition(*type, model);
        } else {
            text += MakeStructOrClassDefinition(*type, model);
        }
    }
    text += "#pragma pack(pop)\n\n";
    text += "} // namespace Kuru::RL\n";
    return text;
}

[[nodiscard]] std::vector<const TypeInfo*> MakeTopologicalTypeList(const DumpModel& model) {
    std::unordered_map<std::string, const TypeInfo*> byName;
    for (const TypeInfo& type : model.Types) {
        byName[type.FullName] = &type;
    }

    std::vector<const TypeInfo*> ordered;
    std::unordered_set<std::string> visiting;
    std::unordered_set<std::string> visited;

    auto visit = [&](auto&& self, const TypeInfo& type) -> void {
        if (IsManualType(type.FullName) || visited.contains(type.FullName)) {
            return;
        }
        if (!visiting.insert(type.FullName).second) {
            return;
        }
        for (const PropertyInfo& property : type.Properties) {
            if (property.Kind != "StructProperty" || property.StructName.empty() || IsManualType(property.StructName)) {
                continue;
            }
            if (const auto dependency = byName.find(property.StructName); dependency != byName.end() &&
                dependency->second->ClassName == "ScriptStruct") {
                self(self, *dependency->second);
            }
        }
        if (!type.SuperName.empty() && type.SuperName != "None" && !IsManualType(type.SuperName)) {
            if (const auto super = byName.find(type.SuperName); super != byName.end()) {
                self(self, *super->second);
            }
        }
        visiting.erase(type.FullName);
        visited.insert(type.FullName);
        ordered.push_back(&type);
    };

    for (const TypeInfo& type : model.Types) if (type.ClassName == "Enum") visit(visit, type);
    for (const TypeInfo& type : model.Types) if (type.ClassName == "ScriptStruct") visit(visit, type);
    for (const TypeInfo& type : model.Types) {
        if (type.ClassName == "Class") {
            visit(visit, type);
        }
    }
    return ordered;
}

[[nodiscard]] std::string MakeClassesHeader(const DumpModel& model) {
    std::string text;
    text += "#pragma once\n\n";
    text += "#include <Generated/ForwardDeclarations.hpp>\n\n";
    text += "#include <array>\n#include <cstdint>\n\n";
    text += "namespace Kuru::RL {\n\n";
    text += "#pragma pack(push, 1)\n\n";
    for (const TypeInfo* type : MakeTopologicalTypeList(model)) {
        if (type->ClassName == "Enum") {
            text += MakeEnumDefinition(*type, model);
        } else {
            text += MakeStructOrClassDefinition(*type, model);
        }
    }
    text += "#pragma pack(pop)\n\n";
    text += "} // namespace Kuru::RL\n";
    return text;
}

[[nodiscard]] std::string MakeBaseExtensionsHeader(const DumpModel& model) {
    std::string text;
    text += "#pragma once\n\n";
    text += "#include <KuruSDKBase.hpp>\n\n";
    text += "#include <array>\n#include <cstdint>\n\n";
    text += "namespace Kuru::RL::BaseExtensions {\n\n";
    text += "// Reflected UFunction wrappers for classes owned by Base SDK.\n";
    text += "// They are free functions on purpose: Base SDK keeps the manual C++ type canonical,\n";
    text += "// while generated reflection supplies update-specific ProcessEvent wrappers.\n\n";

    text += "#pragma pack(push, 1)\n\n";
    for (const TypeInfo& type : model.Types) {
        if (!IsManualType(type.FullName) || type.Functions.empty() ||
            Kuru::SDK::Metadata::FindBaseReflectedCppName(type.FullName).empty()) {
            continue;
        }

        text += "namespace ";
        text += SanitizeIdentifier(type.FullName);
        text += " {\n\n";

        std::unordered_set<std::string> functionNames;
        for (const FunctionInfo& fn : type.Functions) {
            AppendBaseExtensionWrapper(text, type, fn, model, functionNames);
        }

        text += "} // namespace ";
        text += SanitizeIdentifier(type.FullName);
        text += "\n\n";
    }

    text += "#pragma pack(pop)\n\n";
    text += "} // namespace Kuru::RL::BaseExtensions\n";
    return text;
}

[[nodiscard]] std::string MakeAllHeader() {
    std::string text;
    text += "#pragma once\n\n";
    text += "#include <Generated/ManualTypes.hpp>\n";
    text += "#include <Generated/ReflectedTypes.hpp>\n";
    text += "#include <Generated/ForwardDeclarations.hpp>\n";
    text += "#include <Generated/Classes.hpp>\n";
    text += "#include <Generated/BaseExtensions.hpp>\n";
    return text;
}

[[nodiscard]] std::string MakeGeneratedSdkHeader() {
    return "#pragma once\n\n"
           "#include <KuruSDKBase.hpp>\n"
           "#include <Generated/All.hpp>\n";
}

[[nodiscard]] std::string MakeApiHeader() {
    return "#pragma once\n\n"
           "#include <KuruSDK.hpp>\n\n"
           "// Opt-in modder convenience surface.\n"
           "namespace ku = Kuru;\n"
           "using namespace Kuru::SDK;\n"
           "using namespace Kuru::RL;\n";
}

[[nodiscard]] std::string MakeReadme(const DumpModel& model) {
    std::size_t manualSkipped = 0;
    std::size_t classes = 0;
    std::size_t structs = 0;
    std::size_t enums = 0;
    for (const TypeInfo& type : model.Types) {
        manualSkipped += IsManualType(type.FullName) ? 1 : 0;
        classes += !IsManualType(type.FullName) && type.ClassName == "Class" ? 1 : 0;
        structs += !IsManualType(type.FullName) && type.ClassName == "ScriptStruct" ? 1 : 0;
        enums += !IsManualType(type.FullName) && type.ClassName == "Enum" ? 1 : 0;
    }

    std::string text;
    text += "# Generated KuruSDK\n\n";
    text += "This directory was emitted by the injected `KuruSDKGen`, straight from the live game's reflection.\n\n";
    text += "- Base SDK types are included through `KuruSDKBase.hpp` and are never duplicated.\n";
    text += "- Reflected classes, structs, enums, typed params wrappers, `StaticClass`, and default-object helpers are emitted under `Generated/Packages`.\n";
    text += "- Generated wrappers call Base SDK `UObject::ProcessEvent` and cache the reflected `UFunction*` by full name.\n";
    text += "- Counts: classes=";
    text += std::to_string(classes);
    text += " structs=";
    text += std::to_string(structs);
    text += " enums=";
    text += std::to_string(enums);
    text += " manual_skipped=";
    text += std::to_string(manualSkipped);
    text += " name_collisions=";
    text += std::to_string(model.NameCollisions);
    text += "\n";
    return text;
}

[[nodiscard]] std::string MakeNameCollisionReport(const DumpModel& model) {
    std::string text = "# Generated Name Collisions\n\n";
    text += "Only real C++ identifier collisions are disambiguated. The generated name uses the package name, then a numeric suffix only if still needed.\n\n";
    text += "| Reflected full name | Generated C++ name |\n|---|---|\n";
    for (const auto& [fullName, cppName] : model.NameCollisionMappings) {
        text += "| `" + fullName + "` | `" + cppName + "` |\n";
    }
    return text;
}

bool WriteTextFile(const std::filesystem::path& path, std::string_view text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        std::cerr << "failed to open output file: " << path << '\n';
        return false;
    }
    out << text;
    return true;
}

bool EmitSdk(const DumpModel& model, const std::filesystem::path& outputPath) {
    std::map<std::string, std::vector<const TypeInfo*>> byPackage;
    for (const TypeInfo& type : model.Types) {
        if (IsManualType(type.FullName)) {
            continue;
        }
        byPackage[type.PackageName].push_back(&type);
    }

    std::vector<std::string> packages;
    packages.reserve(byPackage.size());
    for (auto& [package, types] : byPackage) {
        packages.push_back(package);
        std::stable_sort(types.begin(), types.end(), [](const TypeInfo* lhs, const TypeInfo* rhs) {
            if (lhs->ClassName != rhs->ClassName) {
                return lhs->ClassName < rhs->ClassName;
            }
            return lhs->FullName < rhs->FullName;
        });
        if (!WriteTextFile(outputPath / "Generated" / "Packages" / (SanitizeIdentifier(package) + ".hpp"),
                           MakePackageHeader(package, types, model))) {
            return false;
        }
    }

    return WriteTextFile(outputPath / "Generated" / "ManualTypes.hpp", MakeManualTypesHeader()) &&
           WriteTextFile(outputPath / "Generated" / "ReflectedTypes.hpp", MakeReflectedTypesHeader(model)) &&
           WriteTextFile(outputPath / "Generated" / "ForwardDeclarations.hpp", MakeForwardDeclarations(model)) &&
           WriteTextFile(outputPath / "Generated" / "BaseExtensions.hpp", MakeBaseExtensionsHeader(model)) &&
           WriteTextFile(outputPath / "Generated" / "Classes.hpp", MakeClassesHeader(model)) &&
           WriteTextFile(outputPath / "Generated" / "All.hpp", MakeAllHeader()) &&
           WriteTextFile(outputPath / "KuruSDK.hpp", MakeGeneratedSdkHeader()) &&
           WriteTextFile(outputPath / "Kuru" / "Api.hpp", MakeApiHeader()) &&
           WriteTextFile(outputPath / "Generated" / "NameCollisions.md", MakeNameCollisionReport(model)) &&
           WriteTextFile(outputPath / "README.md", MakeReadme(model));
}

} // namespace

namespace Kuru::SdkGen {
// Generate the reflected SDK into outputPath from the in-memory model the runtime's reflection walk produced
bool Generate(const Kuru::Common::DumpSchema::DumpDocument& document, const std::filesystem::path& outputPath) {
    try {
        const DumpModel model = BuildModel(document);
        return EmitSdk(model, outputPath);
    } catch (const std::exception&) {
        return false;
    }
}
}  // namespace Kuru::SdkGen
