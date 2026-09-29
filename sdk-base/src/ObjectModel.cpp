#include <Core/ObjectModel.hpp>

#include <Core/Runtime.hpp>

#include <stdexcept>
#include <unordered_map>

namespace Kuru::SDK {
namespace {
[[nodiscard]] std::wstring Widen(std::string_view value) {
    std::wstring result;
    result.reserve(value.size());
    for (char ch : value) {
        result.push_back(static_cast<unsigned char>(ch));
    }
    return result;
}

[[nodiscard]] UClass* FindClassByName(const char* fullName) {
    return UObject::FindClass(fullName);
}

}

TArray<UObject*>* UObject::GetGlobalObjects() { return Kuru::SDK::GetGlobalObjects(); }

UClass* UObject::StaticClass() { return FindClassByName("Core.Object"); }
UClass* UField::StaticClass() { return FindClassByName("Core.Field"); }
UClass* UStruct::StaticClass() { return FindClassByName("Core.Struct"); }
UClass* UScriptStruct::StaticClass() { return FindClassByName("Core.ScriptStruct"); }
UClass* UFunction::StaticClass() { return FindClassByName("Core.Function"); }
UClass* UState::StaticClass() { return FindClassByName("Core.State"); }
UClass* UClass::StaticClass() { return FindClassByName("Core.Class"); }
UClass* UProperty::StaticClass() { return FindClassByName("Core.Property"); }
UClass* UObjectProperty::StaticClass() { return FindClassByName("Core.ObjectProperty"); }
UClass* UClassProperty::StaticClass() { return FindClassByName("Core.ClassProperty"); }
UClass* UByteProperty::StaticClass() { return FindClassByName("Core.ByteProperty"); }
UClass* UBoolProperty::StaticClass() { return FindClassByName("Core.BoolProperty"); }
UClass* UIntProperty::StaticClass() { return FindClassByName("Core.IntProperty"); }
UClass* UQWordProperty::StaticClass() { return FindClassByName("Core.QWordProperty"); }
UClass* USQWordProperty::StaticClass() { return FindClassByName("Core.SQWordProperty"); }
UClass* UFloatProperty::StaticClass() { return FindClassByName("Core.FloatProperty"); }
UClass* UNameProperty::StaticClass() { return FindClassByName("Core.NameProperty"); }
UClass* UStrProperty::StaticClass() { return FindClassByName("Core.StrProperty"); }
UClass* UArrayProperty::StaticClass() { return FindClassByName("Core.ArrayProperty"); }
UClass* UMapProperty::StaticClass() { return FindClassByName("Core.MapProperty"); }
UClass* UStructProperty::StaticClass() { return FindClassByName("Core.StructProperty"); }
UClass* UDelegateProperty::StaticClass() { return FindClassByName("Core.DelegateProperty"); }
UClass* UInterfaceProperty::StaticClass() { return FindClassByName("Core.InterfaceProperty"); }
UClass* UComponentProperty::StaticClass() { return FindClassByName("Core.ComponentProperty"); }
UClass* UEnum::StaticClass() { return FindClassByName("Core.Enum"); }
UClass* UConst::StaticClass() { return FindClassByName("Core.Const"); }
UClass* UMetaData::StaticClass() { return FindClassByName("Core.MetaData"); }

UClass* UObject::FindClass(const std::string& classFullName) {
    // Match by name, cache hits so repeated StaticClass() calls don't rescan all the objects each time.
    static std::unordered_map<std::string, UClass*> cache;
    if (const auto it = cache.find(classFullName); it != cache.end()) {
        return it->second;
    }
    UClass* result = nullptr;
    if (auto* objects = GetGlobalObjects()) {
        for (UObject* object : *objects) {
            if (!object) {
                continue;
            }
            if (object->GetPathName() == classFullName || object->GetFullName() == classFullName) {
                result = reinterpret_cast<UClass*>(object);
                break;
            }
        }
    }
    if (result) {
        cache.emplace(classFullName, result);
    }
    return result;
}

UFunction* UObject::FindFunction(const std::string& functionFullName) {
    return reinterpret_cast<UFunction*>(FindObject(functionFullName, UFunction::StaticClass()));
}

UObject* UObject::StaticFindObject(UClass* objectClass, UObject* objectPackage, const wchar_t* objectName, bool exactClass) {
    if (auto* fn = GetStaticFindObject()) {
        return fn(objectClass, objectPackage, objectName, exactClass);
    }
    return nullptr;
}

UObject* UObject::StaticFindObjectFast(UClass* objectClass, UObject* objectPackage, FName objectName, bool exactClass) {
    if (auto* fn = GetStaticFindObjectFast()) {
        return fn(objectClass, objectPackage, objectName, exactClass);
    }
    (void)objectClass;
    (void)objectPackage;
    (void)objectName;
    (void)exactClass;
    return nullptr;
}

UObject* UObject::StaticLoadObject(UClass* objectClass, UObject* outer, const wchar_t* name, const wchar_t* filename,
    std::uint32_t loadFlags, void* sandbox) {
    if (auto* fn = GetStaticLoadObject()) {
        return fn(objectClass, outer, name, filename, loadFlags, sandbox);
    }
    return nullptr;
}

UObject* UObject::DynamicLoadObject(const wchar_t* objectName, UClass* objectClass, bool mayFail) {
    if (auto* fn = GetDynamicLoadObject()) {
        return fn(objectName, objectClass, mayFail);
    }
    return nullptr;
}

UObject* UObject::FindObject(const std::string& objectFullName, UClass* requiredClass) {
    if (auto* found = StaticFindObject(requiredClass, nullptr, Widen(objectFullName).c_str(), false)) {
        return found;
    }
    auto* objects = GetGlobalObjects();
    if (!objects) {
        return nullptr;
    }
    for (UObject* object : *objects) {
        if (!object) {
            continue;
        }
        if (requiredClass && !object->IsA(requiredClass)) {
            continue;
        }
        if (object->GetFullName() == objectFullName || object->GetPathName() == objectFullName) {
            return object;
        }
    }
    return nullptr;
}

UObject* UObject::FindObject(const char* objectFullName, UClass* requiredClass) {
    return FindObject(std::string(objectFullName ? objectFullName : ""), requiredClass);
}

UObject* UObject::FindObjectChecked(const std::string& objectFullName, UClass* requiredClass) {
    UObject* object = FindObject(objectFullName, requiredClass);
    if (!object) {
        throw std::runtime_error("Kuru SDK object not found: " + objectFullName);
    }
    return object;
}

TArray<UObject*> UObject::FindObjects(const std::string& objectFullName, UClass* requiredClass) {
    TArray<UObject*> result;
    auto* objects = GetGlobalObjects();
    if (!objects) {
        return result;
    }
    for (UObject* object : *objects) {
        if (!object) {
            continue;
        }
        if (requiredClass && !object->IsA(requiredClass)) {
            continue;
        }
        if (objectFullName.empty() || object->GetFullName() == objectFullName || object->GetPathName() == objectFullName) {
            result.Add(object);
        }
    }
    return result;
}

std::string UObject::GetName() const { return Name.ToString(); }
void UObject::GetName(FString& result) const { Name.ToString(result); }
void UObject::AppendName(FString& result) const { Name.AppendString(result); }

std::string UObject::GetNameCPP() const {
    const char* prefix = "U";
    if (Class) {
        const std::string className = Class->GetName();
        if (className == "Class") {
            prefix = "U";
        } else if (className == "Function") {
            prefix = "U";
        }
    }
    return std::string(prefix) + GetName();
}

std::string UObject::GetPathName() const {
    std::string result = GetName();
    for (const UObject* outer = Outer; outer; outer = outer->Outer) {
        result = outer->GetName() + "." + result;
    }
    return result;
}

std::string UObject::GetFullName() const {
    return (Class ? Class->GetName() : std::string("Object")) + " " + GetPathName();
}

std::string UObject::GetPackageName() const {
    const auto* outermost = reinterpret_cast<const UObject*>(GetOutermost());
    return outermost ? outermost->GetName() : std::string();
}

UPackage* UObject::GetOutermost() const {
    const UObject* outermost = this;
    while (outermost && outermost->Outer) {
        outermost = outermost->Outer;
    }
    return reinterpret_cast<UPackage*>(const_cast<UObject*>(outermost));
}

UObject* UObject::GetPackageObj() const {
    return reinterpret_cast<UObject*>(GetOutermost());
}

bool UObject::IsA(const UClass* objectClass) const {
    return Class && objectClass && Class->IsChildOf(objectClass);
}

bool UObject::IsA(std::int32_t objectInternalInteger) const {
    return Class && Class->ObjectInternalInteger == objectInternalInteger;
}

bool UObject::IsValidObject() const noexcept {
    return Class != nullptr && !IsPendingKill();
}

UObject* UObject::GetTypedOuter(UClass* targetClass) const {
    for (UObject* nextOuter = Outer; nextOuter != nullptr; nextOuter = nextOuter->Outer) {
        if (nextOuter->IsA(targetClass)) {
            return nextOuter;
        }
    }
    return nullptr;
}

void UObject::ProcessEvent(UFunction* function, void* params, void* result) {
    if (auto* fn = GetProcessEvent()) {
        fn(this, function, params, result);
    }
}

void UObject::ProcessEventChecked(UFunction* function, void* params, void* result) {
    if (!function) {
        throw std::runtime_error("Kuru SDK ProcessEvent called with null function");
    }
    if (!GetProcessEvent()) {
        throw std::runtime_error("Kuru SDK ProcessEvent runtime binding is not initialized");
    }
    ProcessEvent(function, params, result);
}

void UObject::ProcessInternal(FFrame& stack, void* result) {
    if (auto* fn = GetProcessInternal()) {
        fn(this, stack, result);
    }
}

bool UObject::ScriptConsoleExec(const wchar_t* command, void* outputDevice, UObject* executor) {
    if (auto* fn = GetScriptConsoleExec()) {
        return fn(this, command, outputDevice, executor);
    }
    return false;
}

bool UObject::ProcessRemoteFunction(UFunction* function, void* params, void* stack, UObject* subObject) {
    if (auto* fn = GetProcessRemoteFunction()) {
        return fn(this, function, params, stack, subObject);
    }
    return false;
}

void UObject::ProcessDelegate(FName delegateName, void* params, void* result) {
    if (auto* fn = GetProcessDelegate()) {
        fn(this, delegateName, params, result);
    }
}

void UObject::MarkPendingKill() {
    SetFlags(RF_PendingKill);
}

void UObject::ConditionalBeginDestroy() {
    MarkPendingKill();
}

void UField::Bind() {}

UClass* UField::GetOwnerClass() const {
    for (UObject* outer = Outer; outer; outer = outer->Outer) {
        if (outer->IsA(UClass::StaticClass())) {
            return reinterpret_cast<UClass*>(outer);
        }
    }
    return nullptr;
}

UStruct* UField::GetOwnerStruct() const {
    for (UObject* outer = Outer; outer; outer = outer->Outer) {
        if (outer->IsA(UStruct::StaticClass())) {
            return reinterpret_cast<UStruct*>(outer);
        }
    }
    return nullptr;
}

void UField::AddCppProperty(UProperty* property) {
    if (!property) {
        return;
    }
    property->Next = Next;
    Next = property;
}

void UField::MergeBools() {}

void UStruct::Link(void* archive, bool relinkExistingProperties) {
    (void)archive;
    (void)relinkExistingProperties;
}

void UStruct::SerializeBin(void* archive, std::uint8_t* data, std::int32_t maxReadBytes) const {
    (void)archive;
    (void)data;
    (void)maxReadBytes;
}

void UStruct::SerializeTaggedProperties(void* archive, std::uint8_t* data, UStruct* defaultsStruct,
    std::uint8_t* defaults, std::int32_t defaultsCount) const {
    (void)archive;
    (void)data;
    (void)defaultsStruct;
    (void)defaults;
    (void)defaultsCount;
}

UProperty* UStruct::FindPropertyByName(FName propertyName) const {
    for (UField* field = Children; field; field = field->Next) {
        if (field->Name == propertyName && field->IsA(UProperty::StaticClass())) {
            return reinterpret_cast<UProperty*>(field);
        }
    }
    return SuperStruct ? SuperStruct->FindPropertyByName(propertyName) : nullptr;
}

UFunction* UStruct::FindFunctionByName(FName functionName) const {
    for (UField* field = Children; field; field = field->Next) {
        if (field->Name == functionName && field->IsA(UFunction::StaticClass())) {
            return reinterpret_cast<UFunction*>(field);
        }
    }
    return SuperStruct ? SuperStruct->FindFunctionByName(functionName) : nullptr;
}

void UScriptStruct::AllocateStructDefaults() {}
void UScriptStruct::PropagateStructDefaults() {}

UFunction* UFunction::FindFunction(const std::string& functionFullName) {
    return UObject::FindFunction(functionFullName);
}

UProperty* UFunction::GetReturnProperty() const {
    for (UField* field = Children; field; field = field->Next) {
        auto* property = reinterpret_cast<UProperty*>(field);
        if (field->IsA(UProperty::StaticClass()) && property->Offset == ReturnValueOffset) {
            return property;
        }
    }
    return nullptr;
}

UFunction* UState::FindFunctionByName(FName functionName) const {
    if (UFunction* const* fn = FuncMap.Find(functionName)) {
        return *fn;
    }
    return SuperStruct ? reinterpret_cast<const UState*>(SuperStruct)->FindFunctionByName(functionName) : nullptr;
}

bool UClass::ImplementsInterface(const UClass* someInterface) const {
    for (const FImplementedInterface& interfaceInfo : Interfaces) {
        if (interfaceInfo.Class == someInterface) {
            return true;
        }
    }
    return false;
}

UProperty* UClass::FindPropertyByName(FName propertyName) const {
    return UStruct::FindPropertyByName(propertyName);
}

UFunction* UClass::FindFunctionByName(FName functionName) const {
    return UState::FindFunctionByName(functionName);
}

std::string UProperty::GetNameCPP() const {
    return GetName();
}

FString UProperty::GetCPPType(FString* extendedTypeText, std::uint32_t cppExportFlags) const {
    (void)cppExportFlags;
    if (extendedTypeText) {
        extendedTypeText->Clear();
    }
    return FString::FromAnsi(GetClass() ? GetClass()->GetName() : "Property");
}

bool UProperty::ContainsObjectReference() const {
    return IsA(UObjectProperty::StaticClass()) || IsA(UInterfaceProperty::StaticClass()) || IsA(UComponentProperty::StaticClass());
}

bool UProperty::ContainsInstancedObjectProperty() const {
    return HasAnyPropertyFlags(CPF_EditInline | CPF_Component);
}

bool UProperty::RequiresInit() const {
    return !IsA(UIntProperty::StaticClass()) && !IsA(UFloatProperty::StaticClass()) && !IsA(UBoolProperty::StaticClass());
}

std::int32_t UProperty::GetMinAlignment() const {
    return ElementSize >= 8 ? 8 : ElementSize >= 4 ? 4 : 1;
}

FString UProperty::GetValueAsString(const void* container) const {
    if (!container) {
        return FString();
    }
    return FString::FromAnsi(GetName() + "@0x" + std::to_string(Offset));
}

FName UEnum::GetEnumName(std::int32_t index) const {
    return Names.IsValidIndex(index) ? Names[index] : FName(NAME_None);
}

std::int32_t UEnum::FindEnumIndex(FName enumName) const {
    return Names.FindItemIndex(enumName);
}

std::string UEnum::GetNameStringByIndex(std::int32_t index) const {
    return GetEnumName(index).ToString();
}

std::int32_t UEnum::GetValueByName(FName enumName) const {
    return FindEnumIndex(enumName);
}

const FString& UMetaData::GetValue(const UObject* object, const char* key) const {
    static const FString empty;
    const auto* values = ObjectMetaDataMap.Find(const_cast<UObject*>(object));
    if (!values) {
        return empty;
    }
    const FString* value = values->Find(FName(key, FNAME_Find));
    return value ? *value : empty;
}

const FString& UMetaData::GetValue(const UObject* object, FName key) const {
    static const FString empty;
    const auto* values = ObjectMetaDataMap.Find(const_cast<UObject*>(object));
    if (!values) {
        return empty;
    }
    const FString* value = values->Find(key);
    return value ? *value : empty;
}

bool UMetaData::HasValue(const UObject* object, const char* key) const {
    return !GetValue(object, key).IsEmpty();
}

bool UMetaData::HasValue(const UObject* object, FName key) const {
    return !GetValue(object, key).IsEmpty();
}

bool UMetaData::HasObjectValues(const UObject* object) const {
    return ObjectMetaDataMap.Contains(const_cast<UObject*>(object));
}

void UMetaData::SetObjectValues(const UObject* object, const TMap<FName, FString>& values) {
    ObjectMetaDataMap.Add(const_cast<UObject*>(object), values);
}

void UMetaData::SetValue(const UObject* object, const char* key, const FString& value) {
    SetValue(object, FName(key), value);
}

void UMetaData::SetValue(const UObject* object, FName key, const FString& value) {
    ObjectMetaDataMap.FindOrAdd(const_cast<UObject*>(object)).Add(key, value);
}

bool UMetaData::AttemptParseIntrinsicMetaData(const UClass& classToUse, const UObject& objectToUse,
    UMetaData& metaDataToAddTo) {
    (void)classToUse;
    (void)objectToUse;
    (void)metaDataToAddTo;
    return false;
}

} // namespace Kuru::SDK
