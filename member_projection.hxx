#pragma once

#if __cplusplus < 202302L
#error out of date c++ version, compile with -stdc++=2c
#elif defined(__clang__) && __clang_major__ < 22
#error out of date clang, compile with latest version
#elif !defined(__clang__) && defined(__GNUC__) && __GNUC__ < 14
#error out of date g++, compile with latest version
#elif defined(_MSC_VER) && _MSC_VER < 19
#error out of date msvc, compile with latest version
#elif !defined(__clang__) && !defined(__GNUC__) && !defined(_MSC_VER)
#error compiler unknown, could not detect gcc, clang, or msvc
#else

#include <functional>
#include <meta>
#include <ranges>
#include <type_traits>

namespace irv {
    namespace detail {
        template<typename tp_data_t>
        struct name_or_index {
            tp_data_t m_data;

            auto constexpr static m_is_index = std::integral<tp_data_t>;

            template<std::size_t tp_size>
            constexpr name_or_index(const char (&p_data)[tp_size]) {
                std::ranges::copy(
                    p_data,
                    std::ranges::begin(m_data)
                );
            }

            template<std::size_t tp_size>
            constexpr name_or_index(const std::array<char, tp_size>& p_data) {
                m_data = p_data;
            }
            
            constexpr name_or_index(const std::size_t p_index) : m_data{p_index} {}
        };
        template<std::size_t tp_size>
        name_or_index(const char (&)[tp_size]) ->
        name_or_index<std::array<char, tp_size>>;

        template<std::size_t tp_size>
        name_or_index(const std::array<char, tp_size>&) ->
        name_or_index<std::array<char, tp_size>>;

        name_or_index(const std::size_t) ->
        name_or_index<std::size_t>;

        template<name_or_index tp_name_or_index>
        struct member_fn {
        private:
            template<typename tp_type_t>
            [[nodiscard]]
            auto consteval static get_member() noexcept -> auto {
                auto constexpr static l_members =
                    std::define_static_array(std::meta::members_of(
                        ^^std::remove_cvref_t<tp_type_t>,
                        std::meta::access_context::current()
                    ));
                auto constexpr l_is_valid_member = [](std::meta::info p_info) {
                    return
                        std::meta::has_identifier(p_info) && (
                            std::meta::is_static_member(p_info) ||
                            std::meta::is_function(p_info) ||
                            std::meta::is_nonstatic_data_member(p_info)
                        );
                };
                if constexpr (tp_name_or_index.m_is_index) {
                    auto constexpr static l_valid_members = std::define_static_array(l_members | std::views::filter(l_is_valid_member));
                    if constexpr (tp_name_or_index.m_data < std::ranges::size(l_valid_members))
                        return l_valid_members[tp_name_or_index.m_data];
                    else return;
                }
                else template for (auto constexpr l_member : l_members) {
                    if constexpr (
                        l_is_valid_member(l_member) &&
                        std::meta::identifier_of(l_member) == std::string_view{std::ranges::begin(tp_name_or_index.m_data)}
                    ) {
                        return l_member;
                    }
                }
            }
            template<typename tp_type_t>
            auto constexpr static can_get_member = !std::is_void_v<decltype(get_member<tp_type_t>())>;
        public:

            template<typename tp_type_t>
            requires(
                std::is_class_v<std::remove_cvref_t<tp_type_t>> &&
                can_get_member<tp_type_t> &&
                !std::meta::is_static_member(get_member<tp_type_t>())
            )
            [[nodiscard]]
            auto constexpr operator()(tp_type_t&& p_object)
            const noexcept(noexcept(
                std::invoke(
                    &[:get_member<tp_type_t>():],
                    std::forward<tp_type_t>(p_object)
                )
            )) -> decltype(auto) {
                return std::invoke(
                    &[:get_member<tp_type_t>():],
                    std::forward<tp_type_t>(p_object)
                );
            }

            template<typename tp_type_t>
            requires(
                std::is_class_v<std::remove_cvref_t<tp_type_t>> &&
                can_get_member<tp_type_t> &&
                std::meta::is_static_member(get_member<tp_type_t>()) &&
                !std::meta::is_function(get_member<tp_type_t>())
            )
            [[nodiscard]]
            auto constexpr operator()([[maybe_unused]] tp_type_t&& p_object)
            const noexcept
            -> decltype(auto) {
                return [:get_member<tp_type_t>():];
            }

            template<typename tp_type_t>
            requires(
                std::is_class_v<std::remove_cvref_t<tp_type_t>> &&
                can_get_member<tp_type_t> &&
                std::meta::is_static_member(get_member<tp_type_t>()) &&
                std::meta::is_function(get_member<tp_type_t>())
            )
            [[nodiscard]]
            auto constexpr operator()([[maybe_unused]] tp_type_t&& p_object)
            const noexcept(noexcept(std::invoke([:get_member<tp_type_t>():])))
            -> decltype(std::invoke([:get_member<tp_type_t>():])) {
                return std::invoke([:get_member<tp_type_t>():]);
            }

            template<typename tp_type_t>
            requires(
                std::is_class_v<tp_type_t> &&
                can_get_member<tp_type_t>
            )
            [[nodiscard]]
            auto constexpr operator()(tp_type_t* p_pointer_to_object)
            const noexcept(noexcept((*this)(*p_pointer_to_object)))
            -> decltype((*this)(*p_pointer_to_object)) {
                return (*this)(*p_pointer_to_object);
            }
        };
    }
    template<detail::name_or_index tp_name_or_index>
    auto constexpr member = detail::member_fn<tp_name_or_index>{};

    template<
        typename              tp_type_t,
        detail::name_or_index tp_name_or_index
    >
    concept member_projectable = requires { member<tp_name_or_index>(std::declval<tp_type_t>()); };

    template<
        typename              tp_type_t,
        detail::name_or_index tp_name_or_index
    >
    using member_t = decltype(member<tp_name_or_index>(std::declval<tp_type_t>())());
}

#endif
