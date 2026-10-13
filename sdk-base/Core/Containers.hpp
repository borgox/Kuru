#pragma once

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <initializer_list>
#include <new>
#include <span>
#include <type_traits>
#include <utility>

namespace Kuru::SDK {

class UObject;

[[nodiscard]] void* Reallocate(void* original, std::uint32_t size, std::uint32_t alignment);
void Deallocate(void* original);

template <typename T>
struct TArray {
    using ElementType = T;
    using SizeType = std::int32_t;
    static constexpr SizeType npos = -1;

    T* Data = nullptr;
    SizeType Count = 0;
    SizeType Max = 0;

    constexpr TArray() = default;
    constexpr TArray(T* data, SizeType count, SizeType max) noexcept : Data(data), Count(count), Max(max) {}

    [[nodiscard]] T* data() noexcept { return Data; }
    [[nodiscard]] const T* data() const noexcept { return Data; }
    [[nodiscard]] SizeType size() const noexcept { return Count; }
    [[nodiscard]] SizeType capacity() const noexcept { return Max; }
    [[nodiscard]] SizeType Num() const noexcept { return Count; }
    [[nodiscard]] SizeType MaxCount() const noexcept { return Max; }
    [[nodiscard]] bool empty() const noexcept { return Count <= 0; }
    [[nodiscard]] bool IsEmpty() const noexcept { return empty(); }
    [[nodiscard]] bool IsValidIndex(SizeType index) const noexcept { return index >= 0 && index < Count; }
    [[nodiscard]] bool IsAllocated() const noexcept { return Data != nullptr; }

    [[nodiscard]] T& operator[](SizeType index) noexcept { return Data[index]; }
    [[nodiscard]] const T& operator[](SizeType index) const noexcept { return Data[index]; }
    [[nodiscard]] T& operator()(SizeType index) noexcept { return Data[index]; }
    [[nodiscard]] const T& operator()(SizeType index) const noexcept { return Data[index]; }
    [[nodiscard]] T& Last(SizeType indexFromTheEnd = 0) noexcept { return Data[Count - indexFromTheEnd - 1]; }
    [[nodiscard]] const T& Last(SizeType indexFromTheEnd = 0) const noexcept { return Data[Count - indexFromTheEnd - 1]; }

    [[nodiscard]] T* begin() noexcept { return Data; }
    [[nodiscard]] T* end() noexcept { return Data ? Data + Count : nullptr; }
    [[nodiscard]] const T* begin() const noexcept { return Data; }
    [[nodiscard]] const T* end() const noexcept { return Data ? Data + Count : nullptr; }

    [[nodiscard]] std::span<T> span() noexcept {
        return (Data && Count > 0) ? std::span<T>(Data, static_cast<std::size_t>(Count)) : std::span<T>();
    }

    [[nodiscard]] std::span<const T> span() const noexcept {
        return (Data && Count > 0) ? std::span<const T>(Data, static_cast<std::size_t>(Count)) : std::span<const T>();
    }

    [[nodiscard]] SizeType FindItemIndex(const T& item) const {
        for (SizeType index = 0; index < Count; ++index) {
            if (Data[index] == item) {
                return index;
            }
        }
        return npos;
    }

    [[nodiscard]] bool Contains(const T& item) const { return FindItemIndex(item) != npos; }

    void ClearView() noexcept {
        Data = nullptr;
        Count = 0;
        Max = 0;
    }

    void ResetView(T* data = nullptr, SizeType count = 0, SizeType max = 0) noexcept {
        Data = data;
        Count = count;
        Max = max;
    }

    void Add(const T& item);
    void Add(T&& item);
    SizeType AddZeroed(SizeType count = 1);
    SizeType AddUninitialized(SizeType count = 1);
    void Empty(SizeType slack = 0);
    void Reset(SizeType newSize = 0);
    void Remove(SizeType index, SizeType count = 1);
    void RemoveSwap(SizeType index, SizeType count = 1);
    void Insert(const T& item, SizeType index);
    void Reserve(SizeType capacity);
    void SetNum(SizeType newCount);
    void SetNumZeroed(SizeType newCount);
    template <typename Predicate>
    [[nodiscard]] T* FindByPredicate(Predicate predicate) {
        for (T& item : *this) {
            if (predicate(item)) {
                return &item;
            }
        }
        return nullptr;
    }
    template <typename Predicate>
    [[nodiscard]] SizeType IndexOfByPredicate(Predicate predicate) const {
        for (SizeType index = 0; index < Count; ++index) {
            if (predicate(Data[index])) {
                return index;
            }
        }
        return npos;
    }
    template <typename Compare>
    void Sort(Compare compare) {
        if (Data && Count > 1) {
            std::sort(Data, Data + Count, compare);
        }
    }
};

template <typename T>
void TArray<T>::Reserve(SizeType capacity) {
    if (capacity <= Max) {
        return;
    }
    Data = static_cast<T*>(Reallocate(Data, static_cast<std::uint32_t>(sizeof(T) * capacity), alignof(T)));
    Max = capacity;
}

template <typename T>
typename TArray<T>::SizeType TArray<T>::AddUninitialized(SizeType count) {
    const SizeType oldCount = Count;
    if (count <= 0) {
        return oldCount;
    }
    Reserve(Count + count);
    Count += count;
    return oldCount;
}

template <typename T>
typename TArray<T>::SizeType TArray<T>::AddZeroed(SizeType count) {
    const SizeType index = AddUninitialized(count);
    for (SizeType i = 0; i < count; ++i) {
        new (&Data[index + i]) T{};
    }
    return index;
}

template <typename T>
void TArray<T>::Add(const T& item) {
    const SizeType index = AddUninitialized(1);
    new (&Data[index]) T(item);
}

template <typename T>
void TArray<T>::Add(T&& item) {
    const SizeType index = AddUninitialized(1);
    new (&Data[index]) T(std::move(item));
}

template <typename T>
void TArray<T>::Empty(SizeType slack) {
    if constexpr (!std::is_trivially_destructible_v<T>) {
        for (SizeType index = 0; index < Count; ++index) {
            Data[index].~T();
        }
    }
    Count = 0;
    if (slack <= 0) {
        Deallocate(Data);
        Data = nullptr;
        Max = 0;
        return;
    }
    Reserve(slack);
}

template <typename T>
void TArray<T>::Reset(SizeType newSize) {
    if constexpr (!std::is_trivially_destructible_v<T>) {
        for (SizeType index = 0; index < Count; ++index) {
            Data[index].~T();
        }
    }
    Count = 0;
    Reserve(newSize);
}

template <typename T>
void TArray<T>::Remove(SizeType index, SizeType count) {
    if (!IsValidIndex(index) || count <= 0) {
        return;
    }
    if (index + count > Count) {
        count = Count - index;
    }
    if constexpr (!std::is_trivially_destructible_v<T>) {
        for (SizeType i = 0; i < count; ++i) {
            Data[index + i].~T();
        }
    }
    for (SizeType i = index + count; i < Count; ++i) {
        Data[i - count] = std::move(Data[i]);
    }
    Count -= count;
}

template <typename T>
void TArray<T>::RemoveSwap(SizeType index, SizeType count) {
    if (!IsValidIndex(index) || count <= 0) {
        return;
    }
    if (index + count > Count) {
        count = Count - index;
    }
    for (SizeType i = 0; i < count; ++i) {
        const SizeType removeIndex = index + i;
        const SizeType lastIndex = Count - 1 - i;
        if (removeIndex < lastIndex) {
            Data[removeIndex] = std::move(Data[lastIndex]);
        }
    }
    Count -= count;
}

template <typename T>
void TArray<T>::Insert(const T& item, SizeType index) {
    if (index < 0) {
        index = 0;
    }
    if (index > Count) {
        index = Count;
    }
    AddUninitialized(1);
    for (SizeType i = Count - 1; i > index; --i) {
        Data[i] = std::move(Data[i - 1]);
    }
    Data[index] = item;
}

template <typename T>
void TArray<T>::SetNum(SizeType newCount) {
    if (newCount < Count) {
        Remove(newCount, Count - newCount);
        return;
    }
    AddUninitialized(newCount - Count);
}

template <typename T>
void TArray<T>::SetNumZeroed(SizeType newCount) {
    if (newCount < Count) {
        Remove(newCount, Count - newCount);
        return;
    }
    AddZeroed(newCount - Count);
}

template <typename Key, typename Value>
struct TPair {
    Key First{};
    Value Second{};

    constexpr TPair() = default;
    constexpr TPair(const Key& key, const Value& value) : First(key), Second(value) {}

    [[nodiscard]] constexpr Key& KeyValue() noexcept { return First; }
    [[nodiscard]] constexpr const Key& KeyValue() const noexcept { return First; }
    [[nodiscard]] constexpr Value& ValueValue() noexcept { return Second; }
    [[nodiscard]] constexpr const Value& ValueValue() const noexcept { return Second; }
};

template <typename T>
struct TTransArray : public TArray<T> {
    using Super = TArray<T>;

    UObject* Owner = nullptr;

    constexpr TTransArray() = default;
    constexpr TTransArray(UObject* owner, T* data = nullptr, typename Super::SizeType count = 0,
        typename Super::SizeType max = 0) noexcept
        : Super(data, count, max), Owner(owner) {}

    [[nodiscard]] UObject* GetOwner() const noexcept { return Owner; }
    void SetOwner(UObject* owner) noexcept { Owner = owner; }
    void ModifyItem(typename Super::SizeType index);
    void ModifyAllItems();
};

template <typename T>
using TArrayNoInit = TArray<T>;

template <typename T>
using TIndirectArray = TArray<T*>;

struct FBitReference {
    std::uint32_t* Data = nullptr;
    std::uint32_t Mask = 0;
};

struct FSetElementId {
    std::int32_t Index = -1;

    [[nodiscard]] bool IsValidId() const noexcept { return Index >= 0; }
};

struct FSparseArrayAllocationInfo {
    std::int32_t Index = -1;
    void* Pointer = nullptr;
};

template <typename T>
struct TLinkedList {
    TLinkedList* Next = nullptr;
    TLinkedList** PrevLink = nullptr;
    T Element{};

    [[nodiscard]] T& operator*() noexcept { return Element; }
    [[nodiscard]] const T& operator*() const noexcept { return Element; }
};

template <typename T>
struct TDoubleLinkedListNode {
    T Value{};
    TDoubleLinkedListNode* NextNode = nullptr;
    TDoubleLinkedListNode* PrevNode = nullptr;
};

template <typename T>
struct TDoubleLinkedList {
    using NodeType = TDoubleLinkedListNode<T>;

    NodeType* HeadNode = nullptr;
    NodeType* TailNode = nullptr;
    std::int32_t ListSize = 0;

    [[nodiscard]] bool IsEmpty() const noexcept { return ListSize == 0; }
};

template <typename T>
struct TSparseArray {
    union ElementOrFreeListLink {
        T ElementData;
        struct {
            std::int32_t PrevFreeIndex;
            std::int32_t NextFreeIndex;
        } FreeList;

        ElementOrFreeListLink() {}
        ~ElementOrFreeListLink() {}
    };

    TArray<ElementOrFreeListLink> Data;
    TArray<std::uint32_t> AllocationFlags;
    std::int32_t FirstFreeIndex = -1;
    std::int32_t NumFreeIndices = 0;

    [[nodiscard]] std::int32_t Num() const noexcept { return Data.Count - NumFreeIndices; }
    [[nodiscard]] bool IsValidIndex(std::int32_t index) const noexcept {
        return index >= 0 && index < Data.Count;
    }

    [[nodiscard]] T& operator[](std::int32_t index) noexcept { return Data[index].ElementData; }
    [[nodiscard]] const T& operator[](std::int32_t index) const noexcept { return Data[index].ElementData; }
};

template <typename ElementType>
struct TSetElement {
    ElementType Value{};
    std::int32_t HashNextId = -1;
    std::int32_t HashIndex = 0;
};

template <typename Key, typename Value>
struct TMap {
    using ElementType = TPair<Key, Value>;

    TArray<ElementType> Elements;
    std::uintptr_t IndirectData = 0;
    std::int32_t InlineData[4]{};
    std::int32_t NumBits = 0;
    std::int32_t MaxBits = 0;
    std::int32_t FirstFreeIndex = 0;
    std::int32_t NumFreeIndices = 0;
    std::int64_t InlineHash = 0;
    std::int32_t* Hash = nullptr;
    std::int32_t HashCount = 0;

    [[nodiscard]] std::int32_t Num() const noexcept { return Elements.Count; }
    [[nodiscard]] bool IsEmpty() const noexcept { return Num() == 0; }
    [[nodiscard]] bool IsValidIndex(std::int32_t index) const noexcept { return Elements.IsValidIndex(index); }
    [[nodiscard]] ElementType& operator[](std::int32_t index) noexcept { return Elements[index]; }
    [[nodiscard]] const ElementType& operator[](std::int32_t index) const noexcept { return Elements[index]; }

    [[nodiscard]] Value* Find(const Key& key) noexcept {
        for (ElementType& element : Elements) {
            if (element.First == key) {
                return &element.Second;
            }
        }
        return nullptr;
    }

    [[nodiscard]] const Value* Find(const Key& key) const noexcept {
        for (const ElementType& element : Elements) {
            if (element.First == key) {
                return &element.Second;
            }
        }
        return nullptr;
    }

    [[nodiscard]] bool Contains(const Key& key) const noexcept { return Find(key) != nullptr; }
    [[nodiscard]] Value& FindChecked(const Key& key) { return *Find(key); }
    [[nodiscard]] const Value& FindChecked(const Key& key) const { return *Find(key); }
    [[nodiscard]] Value& FindOrAdd(const Key& key) {
        if (Value* value = Find(key)) {
            return *value;
        }
        Elements.Add(ElementType{key, Value{}});
        return Elements.Last().Second;
    }
    void Add(const Key& key, const Value& value) {
        if (Value* existing = Find(key)) {
            *existing = value;
            return;
        }
        Elements.Add(ElementType{key, value});
    }
    bool Remove(const Key& key) {
        for (std::int32_t index = 0; index < Elements.Count; ++index) {
            if (Elements[index].First == key) {
                Elements.Remove(index);
                return true;
            }
        }
        return false;
    }
    void GenerateKeyArray(TArray<Key>& outKeys) const {
        outKeys.Reset(Elements.Count);
        for (const ElementType& element : Elements) {
            outKeys.Add(element.First);
        }
    }
    void GenerateValueArray(TArray<Value>& outValues) const {
        outValues.Reset(Elements.Count);
        for (const ElementType& element : Elements) {
            outValues.Add(element.Second);
        }
    }
    [[nodiscard]] ElementType* begin() noexcept { return Elements.begin(); }
    [[nodiscard]] ElementType* end() noexcept { return Elements.end(); }
    [[nodiscard]] const ElementType* begin() const noexcept { return Elements.begin(); }
    [[nodiscard]] const ElementType* end() const noexcept { return Elements.end(); }
};

template <typename T>
struct TSet {
    using SetElementType = TSetElement<T>;

    TSparseArray<SetElementType> Elements;
    TArray<std::int32_t> Hash;
    std::int32_t HashSize = 0;

    [[nodiscard]] std::int32_t Num() const noexcept { return Elements.Num(); }
    [[nodiscard]] bool IsEmpty() const noexcept { return Num() == 0; }
    [[nodiscard]] bool IsValidIndex(std::int32_t index) const noexcept { return Elements.IsValidIndex(index); }
    [[nodiscard]] T* Find(const T& value) noexcept {
        for (std::int32_t index = 0; index < Elements.Data.Count; ++index) {
            if (Elements.IsValidIndex(index) && Elements[index].Value == value) {
                return &Elements[index].Value;
            }
        }
        return nullptr;
    }
    [[nodiscard]] const T* Find(const T& value) const noexcept {
        for (std::int32_t index = 0; index < Elements.Data.Count; ++index) {
            if (Elements.IsValidIndex(index) && Elements[index].Value == value) {
                return &Elements[index].Value;
            }
        }
        return nullptr;
    }
    [[nodiscard]] bool Contains(const T& value) const noexcept { return Find(value) != nullptr; }
};

template <typename Key, typename Value>
using TMultiMap = TMap<Key, Value>;

template <typename Key, typename Value>
using TLookupMap = TMap<Key, Value>;

template <typename T, std::size_t N>
struct TStaticArray {
    T Elements[N]{};

    [[nodiscard]] constexpr std::size_t Num() const noexcept { return N; }
    [[nodiscard]] constexpr bool IsValidIndex(std::size_t index) const noexcept { return index < N; }
    [[nodiscard]] constexpr T& operator[](std::size_t index) noexcept { return Elements[index]; }
    [[nodiscard]] constexpr const T& operator[](std::size_t index) const noexcept { return Elements[index]; }
    [[nodiscard]] constexpr T* begin() noexcept { return Elements; }
    [[nodiscard]] constexpr T* end() noexcept { return Elements + N; }
    [[nodiscard]] constexpr const T* begin() const noexcept { return Elements; }
    [[nodiscard]] constexpr const T* end() const noexcept { return Elements + N; }
};

template <typename T>
struct TChunkedArray {
    TArray<T*> Chunks;
    std::int32_t Count = 0;

    [[nodiscard]] std::int32_t Num() const noexcept { return Count; }
};

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif

static_assert(sizeof(TArray<void*>) == 0x10);
static_assert(sizeof(TTransArray<void*>) == 0x18);
static_assert(offsetof(TArray<void*>, Data) == 0x0);
static_assert(offsetof(TArray<void*>, Count) == 0x8);
static_assert(offsetof(TArray<void*>, Max) == 0xC);
static_assert(offsetof(TTransArray<void*>, Owner) == 0x10);
static_assert(std::is_standard_layout_v<TArray<void*>>);
static_assert(sizeof(FSetElementId) == 0x04);
static_assert(sizeof(FSparseArrayAllocationInfo) == 0x10);
static_assert(sizeof(TMap<std::int32_t, std::int32_t>) == 0x50);

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

} // namespace Kuru::SDK
