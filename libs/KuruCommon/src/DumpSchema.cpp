#include "Kuru/Common/DumpSchema.hpp"

#include <iomanip>
#include <charconv>
#include <cctype>
#include <fstream>
#include <map>
#include <optional>
#include <sstream>
#include <variant>

namespace Kuru::Common::DumpSchema {

namespace {

struct JsonValue {
    using Object = std::map<std::string, JsonValue, std::less<>>;
    using Array = std::vector<JsonValue>;
    std::variant<std::nullptr_t, bool, double, std::string, Array, Object> value;
};

class JsonReader {
public:
    explicit JsonReader(std::string_view text) : text_(text) {}

    [[nodiscard]] std::optional<JsonValue> Read(std::string& error) {
        SkipWhitespace();
        auto value = ReadValue(error);
        SkipWhitespace();
        if (value && position_ != text_.size()) {
            error = "unexpected trailing JSON data";
            return std::nullopt;
        }
        return value;
    }

    [[nodiscard]] std::optional<JsonValue> ReadNext(std::string& error) {
        SkipWhitespace();
        if (position_ < text_.size() && text_[position_] == ',') {
            ++position_;
            SkipWhitespace();
        }
        if (position_ == text_.size() || text_[position_] == ']') return std::nullopt;
        return ReadValue(error);
    }

private:
    void SkipWhitespace() {
        while (position_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[position_]))) {
            ++position_;
        }
    }

    [[nodiscard]] std::optional<JsonValue> ReadValue(std::string& error) {
        SkipWhitespace();
        if (position_ == text_.size()) {
            error = "unexpected end of JSON";
            return std::nullopt;
        }
        switch (text_[position_]) {
        case '{': return ReadObject(error);
        case '[': return ReadArray(error);
        case '"': {
            auto value = ReadString(error);
            return value ? std::optional<JsonValue>(JsonValue{std::move(*value)}) : std::nullopt;
        }
        case 't': return ReadLiteral("true", JsonValue{true}, error);
        case 'f': return ReadLiteral("false", JsonValue{false}, error);
        case 'n': return ReadLiteral("null", JsonValue{nullptr}, error);
        default: return ReadNumber(error);
        }
    }

    [[nodiscard]] std::optional<JsonValue> ReadObject(std::string& error) {
        ++position_;
        JsonValue::Object object;
        SkipWhitespace();
        if (position_ < text_.size() && text_[position_] == '}') {
            ++position_;
            return JsonValue{std::move(object)};
        }
        while (position_ < text_.size()) {
            if (text_[position_] != '"') {
                error = "expected JSON object key";
                return std::nullopt;
            }
            auto key = ReadString(error);
            if (!key) return std::nullopt;
            SkipWhitespace();
            if (position_ == text_.size() || text_[position_++] != ':') {
                error = "expected ':' after JSON key";
                return std::nullopt;
            }
            auto value = ReadValue(error);
            if (!value) return std::nullopt;
            object.emplace(std::move(*key), std::move(*value));
            SkipWhitespace();
            if (position_ < text_.size() && text_[position_] == '}') {
                ++position_;
                return JsonValue{std::move(object)};
            }
            if (position_ == text_.size() || text_[position_++] != ',') {
                error = "expected ',' in JSON object";
                return std::nullopt;
            }
            SkipWhitespace();
        }
        error = "unterminated JSON object";
        return std::nullopt;
    }

    [[nodiscard]] std::optional<JsonValue> ReadArray(std::string& error) {
        ++position_;
        JsonValue::Array array;
        SkipWhitespace();
        if (position_ < text_.size() && text_[position_] == ']') {
            ++position_;
            return JsonValue{std::move(array)};
        }
        while (position_ < text_.size()) {
            auto value = ReadValue(error);
            if (!value) return std::nullopt;
            array.push_back(std::move(*value));
            SkipWhitespace();
            if (position_ < text_.size() && text_[position_] == ']') {
                ++position_;
                return JsonValue{std::move(array)};
            }
            if (position_ == text_.size() || text_[position_++] != ',') {
                error = "expected ',' in JSON array";
                return std::nullopt;
            }
            SkipWhitespace();
        }
        error = "unterminated JSON array";
        return std::nullopt;
    }

    [[nodiscard]] std::optional<std::string> ReadString(std::string& error) {
        if (position_ == text_.size() || text_[position_++] != '"') {
            error = "expected JSON string";
            return std::nullopt;
        }
        std::string output;
        while (position_ < text_.size()) {
            const char ch = text_[position_++];
            if (ch == '"') return output;
            if (ch != '\\') {
                output.push_back(ch);
                continue;
            }
            if (position_ == text_.size()) break;
            switch (text_[position_++]) {
            case '"': output.push_back('"'); break;
            case '\\': output.push_back('\\'); break;
            case '/': output.push_back('/'); break;
            case 'b': output.push_back('\b'); break;
            case 'f': output.push_back('\f'); break;
            case 'n': output.push_back('\n'); break;
            case 'r': output.push_back('\r'); break;
            case 't': output.push_back('\t'); break;
            case 'u':
                if (position_ + 4 > text_.size()) { error = "invalid JSON unicode escape"; return std::nullopt; }
                // Dump strings are ASCII names/paths today. Preserve non-ASCII escapes losslessly enough for parsing.
                position_ += 4;
                output.push_back('?');
                break;
            default: error = "invalid JSON escape"; return std::nullopt;
            }
        }
        error = "unterminated JSON string";
        return std::nullopt;
    }

    [[nodiscard]] std::optional<JsonValue> ReadLiteral(std::string_view literal, JsonValue value, std::string& error) {
        if (text_.substr(position_, literal.size()) != literal) {
            error = "invalid JSON literal";
            return std::nullopt;
        }
        position_ += literal.size();
        return value;
    }

    [[nodiscard]] std::optional<JsonValue> ReadNumber(std::string& error) {
        const std::size_t start = position_;
        while (position_ < text_.size() && std::string_view("-+.0123456789eE").find(text_[position_]) != std::string_view::npos) ++position_;
        double value = 0.0;
        const std::string number(text_.substr(start, position_ - start));
        std::istringstream stream(number);
        stream >> value;
        if (!stream || !stream.eof()) { error = "invalid JSON number"; return std::nullopt; }
        return JsonValue{value};
    }

    std::string_view text_;
    std::size_t position_ = 0;
};

[[nodiscard]] const JsonValue* Member(const JsonValue::Object& object, std::string_view name) {
    const auto it = object.find(name);
    return it == object.end() ? nullptr : &it->second;
}

[[nodiscard]] std::string String(const JsonValue::Object& object, std::string_view name) {
    const JsonValue* value = Member(object, name);
    return value && std::holds_alternative<std::string>(value->value) ? std::get<std::string>(value->value) : std::string{};
}

[[nodiscard]] std::int64_t Integer(const JsonValue::Object& object, std::string_view name, std::int64_t fallback = 0) {
    const JsonValue* value = Member(object, name);
    return value && std::holds_alternative<double>(value->value) ? static_cast<std::int64_t>(std::get<double>(value->value)) : fallback;
}

[[nodiscard]] std::uint64_t HexOrInteger(const JsonValue::Object& object, std::string_view name) {
    const JsonValue* value = Member(object, name);
    if (!value) return 0;
    if (std::holds_alternative<double>(value->value)) return static_cast<std::uint64_t>(std::get<double>(value->value));
    if (!std::holds_alternative<std::string>(value->value)) return 0;
    const std::string& text = std::get<std::string>(value->value);
    std::uint64_t result = 0;
    const char* begin = text.data() + (text.starts_with("0x") ? 2 : 0);
    const auto [_, error] = std::from_chars(begin, text.data() + text.size(), result, 16);
    return error == std::errc{} ? result : 0;
}

[[nodiscard]] PropertyRecord Property(const JsonValue::Object& object) {
    PropertyRecord property;
    property.name = String(object, "name");
    property.kind = String(object, "kind");
    property.cppType = String(object, "cpp_type");
    property.flags = HexOrInteger(object, "flags");
    property.offset = static_cast<std::int32_t>(Integer(object, "offset"));
    property.elementSize = static_cast<std::int32_t>(Integer(object, "element_size"));
    property.arrayDim = static_cast<std::int32_t>(Integer(object, "array_dim", 1));
    property.inner = String(object, "inner");
    property.outer = String(object, "outer");
    property.propertyClass = String(object, "property_class");
    property.metaClass = String(object, "meta_class");
    property.structName = String(object, "struct");
    property.enumName = String(object, "enum");
    property.interfaceClass = String(object, "interface_class");
    property.key = String(object, "key");
    property.value = String(object, "value");
    property.bitMask = String(object, "bit_mask");
    return property;
}

[[nodiscard]] std::vector<PropertyRecord> Properties(const JsonValue::Object& object, std::string_view name) {
    std::vector<PropertyRecord> properties;
    const JsonValue* value = Member(object, name);
    if (!value || !std::holds_alternative<JsonValue::Array>(value->value)) return properties;
    for (const JsonValue& entry : std::get<JsonValue::Array>(value->value)) {
        if (std::holds_alternative<JsonValue::Object>(entry.value)) properties.push_back(Property(std::get<JsonValue::Object>(entry.value)));
    }
    return properties;
}

[[nodiscard]] std::vector<std::string> Strings(const JsonValue::Object& object, std::string_view name) {
    std::vector<std::string> strings;
    const JsonValue* value = Member(object, name);
    if (!value || !std::holds_alternative<JsonValue::Array>(value->value)) return strings;
    for (const JsonValue& entry : std::get<JsonValue::Array>(value->value)) {
        if (std::holds_alternative<std::string>(entry.value)) strings.push_back(std::get<std::string>(entry.value));
    }
    return strings;
}

} // namespace

std::string JsonEscape(std::string_view value) {
    std::ostringstream out;
    for (unsigned char ch : value) {
        switch (ch) {
        case '\\': out << "\\\\"; break;
        case '"': out << "\\\""; break;
        case '\b': out << "\\b"; break;
        case '\f': out << "\\f"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (ch < 0x20) {
                out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(ch)
                    << std::dec << std::setfill(' ');
            } else {
                out << static_cast<char>(ch);
            }
            break;
        }
    }
    return out.str();
}

bool ReadJsonFile(std::string_view path, DumpDocument& document, std::string& error) {
    std::ifstream input(std::string(path), std::ios::binary);
    if (!input) { error = "could not open dump JSON"; return false; }
    const std::string json((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    document = {};
    const std::size_t marker = json.find("\"objects\"");
    if (marker == std::string::npos) { error = "dump has no objects member"; return false; }
    const std::size_t arrayStart = json.find('[', marker);
    if (arrayStart == std::string::npos) { error = "dump objects member is not an array"; return false; }
    JsonReader reader(std::string_view(json).substr(arrayStart + 1));
    while (const auto entry = reader.ReadNext(error)) {
        if (!std::holds_alternative<JsonValue::Object>(entry->value)) { error = "dump objects array contains a non-object"; return false; }
        const auto& source = std::get<JsonValue::Object>(entry->value);
        ObjectRecord record;
        record.name = String(source, "name");
        record.fullName = String(source, "full_name");
        record.packageName = String(source, "package_name");
        record.className = String(source, "class_name");
        record.outerName = String(source, "outer_name");
        record.superName = String(source, "super_name");
        record.propertySize = static_cast<std::int32_t>(Integer(source, "property_size"));
        record.minAlignment = static_cast<std::int32_t>(Integer(source, "min_alignment", 1));
        record.properties = Properties(source, "properties");
        record.enumNames = Strings(source, "enum_names");
        const JsonValue* functions = Member(source, "functions");
        if (functions && std::holds_alternative<JsonValue::Array>(functions->value)) {
            for (const JsonValue& functionValue : std::get<JsonValue::Array>(functions->value)) {
                if (!std::holds_alternative<JsonValue::Object>(functionValue.value)) continue;
                const auto& function = std::get<JsonValue::Object>(functionValue.value);
                FunctionRecord result;
                result.fullName = String(function, "full_name");
                result.flags = HexOrInteger(function, "flags");
                result.parmsSize = static_cast<std::uint16_t>(Integer(function, "parms_size"));
                result.numParms = static_cast<std::uint8_t>(Integer(function, "num_parms"));
                result.returnValueOffset = static_cast<std::uint16_t>(Integer(function, "return_value_offset", 0xFFFF));
                result.params = Properties(function, "params");
                record.functions.push_back(std::move(result));
            }
        }
        document.objects.push_back(std::move(record));
    }
    if (!error.empty()) return false;
    return true;
}

} // namespace Kuru::Common::DumpSchema
