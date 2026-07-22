#pragma once

#if __cplusplus < 202302L
#error out of date c++ version, compile with -stdc++=2c
#elif defined(__clang__) && __clang_major__ < 22
#error out of date clang, compile with latest version
#elif !defined(__clang__) && defined(__GNUC__) && __GNUC__ < 15
#error out of date g++, compile with latest version
#elif defined(_MSC_VER) && _MSC_VER < 19
#error out of date msvc, compile with latest version
#elif !defined(__clang__) && !defined(__GNUC__) && !defined(_MSC_VER)
#error compiler unknown, could not detect gcc, clang, or msvc
#else

#include <concepts>
#include <meta>
#include <ranges>
#include <type_traits>

namespace irv {
    namespace detail {
        template<typename tp_data_t>
        struct name_or_index {
            tp_data_t m_data;

            auto constexpr static is_index = std::integral<tp_data_t>;
            auto constexpr static is_name  = !is_index;

            template<typename tp_const_char_pointer_t>
            requires (!std::integral<tp_const_char_pointer_t>)
            constexpr name_or_index(tp_const_char_pointer_t p_data) noexcept
            : m_data{p_data} {}

            template<std::size_t tp_size>
            constexpr name_or_index(const char (&p_data)[tp_size]) noexcept {
                std::ranges::copy(
                    p_data,
                    std::ranges::begin(m_data)
                );
            }

            template<std::size_t tp_size>
            constexpr name_or_index(const std::array<char, tp_size>& p_data) noexcept {
                m_data = p_data;
            }
            
            constexpr name_or_index(std::size_t p_index) noexcept : m_data{p_index} {}

            [[nodiscard]]
            auto constexpr get_name()
            const noexcept
            -> std::string_view
            requires (!is_index) {
                if constexpr (std::is_pointer_v<tp_data_t>)
                    return std::string_view{m_data};
                else return std::string_view{std::ranges::data(m_data)};
            }
            [[nodiscard]]
            auto constexpr get_index()
            const noexcept
            -> std::size_t
            requires (is_index) {
                return m_data;
            }
        };

        template<typename tp_type_t>
        requires (!std::integral<tp_type_t>)
        name_or_index(tp_type_t) ->
        name_or_index<const char*>;

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
                auto constexpr static l_members = std::define_static_array(
                    std::meta::members_of(
                        ^^std::remove_cvref_t<tp_type_t>,
                        std::meta::access_context::current()
                    ) |
                    std::views::filter([](std::meta::info p_info) {
                        return
                            std::meta::has_identifier(p_info) && (
                                std::meta::is_static_member(p_info) ||
                                std::meta::is_function(p_info) ||
                                std::meta::is_nonstatic_data_member(p_info) ||
                                std::meta::is_function_template(p_info)
                            );
                        })
                );
                if constexpr (tp_name_or_index.is_index) {
                    if constexpr (tp_name_or_index.get_index() < std::ranges::size(l_members))
                        return l_members[tp_name_or_index.get_index()];
                    else return;
                }
                else template for (auto constexpr l_member : l_members)
                    if constexpr (std::meta::identifier_of(l_member) == tp_name_or_index.get_name())
                        return l_member;
            }
            template<typename tp_type_t>
            auto constexpr static can_get_member = !std::is_void_v<decltype(get_member<tp_type_t>())>;

        public:
            // note: data members
            template<typename tp_type_t>
            requires(
                std::is_class_v<std::remove_cvref_t<tp_type_t>> &&
                can_get_member<tp_type_t> &&
                std::meta::is_nonstatic_data_member(get_member<tp_type_t>()) || (
                    std::meta::is_static_member(get_member<tp_type_t>()) &&
                    !std::meta::is_function(get_member<tp_type_t>()) &&
                    !std::meta::is_function_template(get_member<tp_type_t>())
                )
            )
            [[nodiscard]]
            auto constexpr operator()(tp_type_t&& p_object)
            const noexcept(noexcept(std::declval<tp_type_t>().[:get_member<tp_type_t>():]))
            -> decltype(auto) {
                return std::forward<tp_type_t>(p_object).[:get_member<tp_type_t>():];
            }

            // note: member functions, member function templates, and data members invoked
            template<
                typename tp_type_t,
                class... tp_arguments_ts
            >
            requires(
                std::is_class_v<std::remove_cvref_t<tp_type_t>> &&
                can_get_member<tp_type_t>
            )
            [[nodiscard]]
            auto constexpr operator()(
                tp_type_t&&          p_object,
                tp_arguments_ts&&... p_arguments
            )
            const noexcept(noexcept(std::declval<tp_type_t>().[:get_member<tp_type_t>():](std::declval<tp_arguments_ts>()...)))
            -> decltype(std::forward<tp_type_t>(p_object).[:get_member<tp_type_t>():](std::forward<tp_arguments_ts>(p_arguments)...)) {
                return std::forward<tp_type_t>(p_object).[:get_member<tp_type_t>():](std::forward<tp_arguments_ts>(p_arguments)...);
            }
            
            // note: pointer-to-class
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
        detail::name_or_index tp_name_or_index,
        typename              tp_object_t,
        class...              tp_arguments_ts
    >
    concept member_projectable = requires {
        member<tp_name_or_index>(
            std::declval<tp_object_t>(),
            std::declval<tp_arguments_ts>()...
        );
    };

    template<
        detail::name_or_index tp_name_or_index,
        typename              tp_object_t,
        class...              tp_arguments_ts
    >
    auto constexpr nothrow_member_projectable = noexcept(
        member<tp_name_or_index>(
            std::declval<tp_object_t>(),
            std::declval<tp_arguments_ts>()...
        )
    );

    template<
        detail::name_or_index tp_name_or_index,
        typename              tp_object_t,
        class...              tp_arguments_ts
    >
    using member_t = decltype(
        member<tp_name_or_index>(
            std::declval<tp_object_t>(),
            std::declval<tp_arguments_ts>()...
        )
    );
}

#endif
