#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace Kuru::SDK {

struct FString;
struct FName;
class UObject;

class FArchive {
public:
    virtual ~FArchive() = default;

    virtual void Serialize(void* data, std::size_t size) = 0;
    [[nodiscard]] virtual std::size_t Tell() const = 0;
    [[nodiscard]] virtual std::size_t TotalSize() const = 0;
    virtual void Seek(std::size_t offset) = 0;
    [[nodiscard]] virtual bool IsLoading() const = 0;
    [[nodiscard]] virtual bool IsSaving() const = 0;
    [[nodiscard]] virtual bool IsPersistent() const { return false; }
    [[nodiscard]] virtual bool IsTransacting() const { return false; }
    [[nodiscard]] virtual bool IsSerializingDefaults() const { return false; }
    [[nodiscard]] virtual bool WantBinaryPropertySerialization() const { return false; }
    [[nodiscard]] virtual bool ContainsCookedData() const { return false; }
    [[nodiscard]] virtual bool IsFilterEditorOnly() const { return false; }
    [[nodiscard]] virtual bool IsIgnoringArchetypeRef() const { return false; }
    [[nodiscard]] virtual std::uint32_t GetPortFlags() const { return 0; }

    void SerializeBits(void* data, std::size_t bitCount) {
        Serialize(data, (bitCount + 7) / 8);
    }

    void ByteOrderSerialize(void* data, std::size_t size) {
        Serialize(data, size);
    }

    void SerializeInt(std::uint32_t& value, std::uint32_t max);
    [[nodiscard]] bool AtEnd() const { return Tell() >= TotalSize(); }

    template <typename T>
    FArchive& operator<<(T& value) {
        static_assert(std::is_trivially_copyable_v<T>, "Use explicit serializers for non-trivial types");
        Serialize(&value, sizeof(T));
        return *this;
    }
};

class FMemoryArchive final : public FArchive {
public:
    enum class Mode {
        Load,
        Save,
    };

    FMemoryArchive(const void* data, std::size_t size)
        : mode_(Mode::Load), readData_(static_cast<const std::uint8_t*>(data)), size_(size) {}

    explicit FMemoryArchive(std::vector<std::uint8_t>& output)
        : mode_(Mode::Save), writeData_(&output) {}

    void Serialize(void* data, std::size_t size) override {
        if (mode_ == Mode::Load) {
            if (!readData_ || offset_ + size > size_) {
                throw std::out_of_range("FMemoryArchive read past end");
            }
            std::memcpy(data, readData_ + offset_, size);
            offset_ += size;
            return;
        }

        if (!writeData_) {
            throw std::runtime_error("FMemoryArchive has no output buffer");
        }

        const auto oldSize = writeData_->size();
        if (offset_ + size > oldSize) {
            writeData_->resize(offset_ + size);
        }
        std::memcpy(writeData_->data() + offset_, data, size);
        offset_ += size;
    }

    [[nodiscard]] std::size_t Tell() const override { return offset_; }
    [[nodiscard]] std::size_t TotalSize() const override { return mode_ == Mode::Load ? size_ : writeData_->size(); }

    void Seek(std::size_t offset) override {
        if (mode_ == Mode::Load && offset > size_) {
            throw std::out_of_range("FMemoryArchive seek past end");
        }
        offset_ = offset;
    }

    [[nodiscard]] bool IsLoading() const override { return mode_ == Mode::Load; }
    [[nodiscard]] bool IsSaving() const override { return mode_ == Mode::Save; }

private:
    Mode mode_ = Mode::Load;
    const std::uint8_t* readData_ = nullptr;
    std::vector<std::uint8_t>* writeData_ = nullptr;
    std::size_t size_ = 0;
    std::size_t offset_ = 0;
};

FArchive& operator<<(FArchive& archive, FString& value);
FArchive& operator<<(FArchive& archive, FName& value);
FArchive& operator<<(FArchive& archive, UObject*& value);

using FMemoryReader = FMemoryArchive;
using FMemoryWriter = FMemoryArchive;

} // namespace Kuru::SDK
