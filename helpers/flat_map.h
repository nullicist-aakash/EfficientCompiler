#pragma once
#include <cstdint>
#include <array>
#include <optional>
#include <algorithm>

namespace helpers
{
    template <typename Key, typename Value, std::size_t Size>
    class flatmap
    {
        std::array<std::pair<Key, Value>, Size> data{};
        std::size_t cur_size{};

        [[nodiscard]]
        constexpr auto get_val(this auto&& self, const Key& key) noexcept
        {
            const auto itr = std::find_if(self.begin(), self.end(),
                                          [&key](const auto& v) { return v.first == key; });

            if (itr != self.end())
                return std::optional{ &itr->second };

            return decltype(std::optional{ &itr->second }){ std::nullopt };
        }

    public:
        [[nodiscard]]
        constexpr auto contains(this auto&& self, const Key& key) noexcept -> bool
        {
            return self.get_val(key).has_value();
        }

        constexpr auto insert(this auto&& self, const Key& key, const Value& val) noexcept -> void
        {
            auto res = self.get_val(key);
            if (res.has_value())
                *res.value() = val;
            else if (self.cur_size == Size)
                std::terminate();
            else
                self.data[self.cur_size++] = { key, val };
        }

        [[nodiscard]]
        constexpr auto size() const noexcept -> std::size_t
        {
            return cur_size;
        }

        [[nodiscard]]
        constexpr auto& operator[](this auto&& self, const Key& key) noexcept
        {
            return *self.get_val(key).value();
        }

        [[nodiscard]] constexpr auto begin(this auto&& self) noexcept
        {
            return std::begin(self.data);
        }

        [[nodiscard]] constexpr auto end(this auto&& self) noexcept
        {
            return self.begin() + self.cur_size;
        }
    };
}