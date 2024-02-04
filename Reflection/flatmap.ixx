export module helpers:flatmap;
import std;

export template <typename Key, typename Value, std::size_t Size>
class flatmap
{
    std::array<std::pair<Key, Value>, Size> data{};
    std::size_t cur_size{};

    [[nodiscard]]
    constexpr std::optional<Value*> get_val(const Key& key)
    {
        auto bgn = std::begin(data);
        auto ed = std::begin(data) + cur_size;
        const auto itr = std::find_if(bgn, ed,
            [&key](const auto& v) { return v.first == key; });

        if (itr == ed)
            return {};

        return { &itr->second };
    }

    [[nodiscard]]
    constexpr std::optional<const Value*> get_val(const Key& key) const
    {
        auto bgn = begin();
        auto ed = end();
        const auto itr = std::find_if(bgn, ed,
            [&key](const auto& v) { return v.first == key; });

        if (itr == ed)
            return {};

        return { &itr->second };
    }

public:
    [[nodiscard]]
    constexpr bool exists(const Key& key) const
    {
        return get_val(key).has_value();
    }

    [[nodiscard]]
    constexpr Value& at(const Key& key)
    {
        auto res = get_val(key);
        if (!res.has_value())
            throw std::range_error(format("Key not found in the map: {}.", key));
        return *res.value();
    }

    constexpr void insert(const Key& key, const Value& val)
    {
        auto res = get_val(key);
        if (res.has_value())
            *res.value() = val;
        else if (cur_size == Size)
            throw std::range_error("Could not add more elements to map.");
        else
            data[cur_size++] = { key, val };
    }

    [[nodiscard]]
    constexpr const flatmap& get_data_copy() const
    {
        return *this;
    }

    [[nodiscard]]
    constexpr auto size() const
    {
        return cur_size;
    }

    [[nodiscard]]
    constexpr const Value& at(const Key& key) const
    {
        auto res = get_val(key);
        if (!res.has_value())
            throw std::range_error(format("Key not found in the map: {}.", key));
        return *res.value();
    }

    [[nodiscard]] constexpr auto begin() const
    {
        return std::begin(data);
    }

    [[nodiscard]] constexpr auto end() const
    {
        return std::begin(data) + cur_size;
    }
};