///
/// \file   include/csv_co/reader.hpp
/// \author wiluite
/// \brief  CSV_co reader class and facilities.

#pragma once

#include "csv_co/external/mmap.hpp"
#include "csv_co/external/type_name.h"
#include <charconv>

#ifndef _MSC_VER
    #include "csv_co/external/fast_float/fast_float.h"
#else
    #ifdef max
    #undef max
    #undef min
    #endif
#endif

#include "csv_co/external/vince-csv-parser/enum_data_type.h"
#include "external/has_member.hpp"
#include "external/ezgz/ezgz.hpp"
//-----------------------------

#if (IS_CLANG==0)
#ifdef __has_include
    #if __has_include(<memory_resource>)
        #include <memory_resource>
    #endif
#endif
#endif

#if (IS_CLANG==0)
    #ifdef __cpp_lib_memory_resource
    #endif
#endif

#ifdef __cpp_lib_coroutine
    #include <coroutine>
#else
    #if defined (__cpp_impl_coroutine)
        #include <coroutine>
    #else
        #error "Your compiler must have full coroutine support."
    #endif
#endif

#include <optional>
#include <functional>
#include <filesystem>
#include <algorithm>
#include <concepts>
#include <variant>
#include <span>
#include <cmath>
#include <utility>  // for std::exchange
#include <cassert>
#include <sstream>

define_has_member(span);

namespace csv_co {
    static_assert(sizeof(void*) == 8, "You must use a 64-bit environment");
    /// Common CSV cell/field type - usual string type
    using cell_string = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;

    /// honest conversion-to-unquoted-cell type
    struct unquoted_cell_string : cell_string {
        using cell_string::cell_string;
    };

    template<class T>
    concept TrimPolicyConcept = requires(T, cell_string s, unquoted_cell_string u) {
        { T::ret_trim(s) } -> std::convertible_to<cell_string>;
        { T::ret_trim(u) } -> std::convertible_to<unquoted_cell_string>;
    };

    template<char ch>
    struct quote_char {
        constexpr static char value = ch;
    };

    template<class T>
    concept QuoteConcept = requires(T t) {
        { T::value } -> std::convertible_to<char>;
        { t } -> std::convertible_to<quote_char<T::value>>;
    };

    template<char ch>
    struct delimiter {
        constexpr static char value = ch;
    };

    template<class T>
    concept DelimiterConcept = requires(T t) {
        { T::value } -> std::convertible_to<char>;
        { t } -> std::convertible_to<delimiter<T::value>>;
    };

    template<char ch>
    struct line_break {
        constexpr static char value = ch;
    };

    template<class T>
    concept LineBreakConcept = requires(T t) {
        { T::value } -> std::convertible_to<char>;
        { t } -> std::convertible_to<line_break<T::value>>;
    };

    /// Conventional string functions
    namespace string_functions {

        template <char const *list>
        inline auto trim_string(auto & s) {
            s.erase(0, s.find_first_not_of(list));
            s.erase(s.find_last_not_of(list) + 1);
        }

        inline bool devastated(auto const &s) noexcept {
            return (s.find_first_not_of(" \n\r\t") == std::string::npos);
        }

        inline auto begins_with(auto const &s, char ch = '"') noexcept -> std::pair<bool, std::size_t> {
            auto const d = s.find_first_not_of(" \n\r\t");
            return {(d != std::string::npos && s[d] == ch), d};
        }

        inline auto del_last(auto &source, char ch = '"') {
            auto const pos = source.find_last_of(ch);
            assert (pos != std::string::npos);
            auto const & sv = std::string_view(source.begin() + pos + 1, source.end());
            return devastated(std::decay_t<decltype(source)>{sv.begin(), sv.end()}) && (source.erase(pos, 1), true);
        }

        inline auto unquote(cell_string &s, char ch) {
            auto const [ret, pos] = begins_with(s, ch);
            if (ret && del_last(s, ch))
                s.erase(pos, 1);
        }

        inline auto unique_quote(auto &s, char q) {
            auto const last = unique(s.begin(), s.end(), [q](auto const &first, auto const &second) {
                return first == q && second == q;
            });
            s.erase(last, s.end());
        }
    }

    namespace trim_policy {
        struct no_trimming {
        public:
            inline static cell_string ret_trim(cell_string const & cs) {
                return cs;
            }
            inline static unquoted_cell_string ret_trim(unquoted_cell_string const &cs) {
                return cs;
            }
        };

        template<char const *list>
        struct trimming {
        public:
            inline static cell_string ret_trim(cell_string s) {
                string_functions::trim_string<list>(s);
                return s;
            }
            inline static unquoted_cell_string ret_trim(unquoted_cell_string s) {
                string_functions::trim_string<list>(s);
                return s;
            }
        };
        static char constexpr chars[] = " \t\r";
        using alltrim = trimming<chars>;
    }

    template<class T>
    concept MaxFieldSizePolicyConcept = requires(T t, unsigned new_limit) {
        { t.reached() } -> std::convertible_to<bool>;
        { t.reset() } -> std::convertible_to<void>;
    };

    /// Max field size policies
    namespace MFS {
        struct no_trace {
            static bool reached() noexcept { return false; }
            static void reset() noexcept {}
        };

        template<unsigned N = 1024>
        struct trace {
            unsigned accumulator = 0;
            unsigned limit = N;

            inline bool reached() noexcept {
                return (++accumulator > limit) && (accumulator = 0, true);
            }

            inline void reset() noexcept { accumulator = 0; }
            void update_limit(unsigned new_limit) {limit = new_limit;}
        };
    }

    template<class T>
    concept EmptyRowsPolicyConcept = requires(T t, char ch, char linebreak) {
        { t.collect_recent(ch, linebreak) } -> std::convertible_to<void>;
        { t.skip(linebreak) } -> std::convertible_to<unsigned>;
    };

    /// Empty rows policies
    namespace ER {
        struct std_4180 {
            inline void collect_recent(char, char) {}
            static inline unsigned skip(char) noexcept { return false; };
        };

        struct ignore {
            inline void collect_recent(char c, char linebreak) noexcept {
                if (c == '\r' || c == linebreak) {
                    assert("Probably Win/Unix/Mac line breaks present simultaneously" && idx <= 3);
                    buf[idx++] = c;
                }
                else
                    idx = 0;
            }

            inline unsigned skip(char linebreak) noexcept {
                unsigned result {0};
                if (idx == 2 && buf[0] == linebreak && buf[1] == linebreak) {
                    idx = result = 1;
                } else {
                    if (idx == 4 && buf[0] == '\r' && buf[1] == linebreak && buf[2] == '\r' && buf[3] == linebreak)
                        idx = result = 2;
                }
                return result;
            }
        private:
            unsigned idx = 0;
            char buf [4] = {0,0,0,0};
        };
    }

    constexpr bool trim_chars_do_not_conflict_with(auto ch, auto is_trimming_policy) {
        if (is_trimming_policy)
            return std::all_of(std::begin(trim_policy::chars), std::end(trim_policy::chars),[&](auto &elem) {
                return elem != ch;
            });
        else
            return true;
    }

    using double_quotes = quote_char<'"'>;
    using comma_delimiter = delimiter<','>;
    using non_mac_ln_brk = line_break<'\n'>;

    /// Concept for reader<>::header() method's container.
    /// Note, no check for return value: try to call header() when reader<> is a template argument.
    template<typename T> concept has_emplace_back = requires(T t, typename T::value_type/*cell_string*/ s) {
        { t.emplace_back(s) } /* ->std::convertible_to<void>*/;
    };

    template<class T>
    struct empty_t {};

    /// CSV reader class
    template<TrimPolicyConcept TrimPolicy = trim_policy::no_trimming, QuoteConcept Quote = double_quotes, DelimiterConcept Delimiter = comma_delimiter, LineBreakConcept LineBreak = non_mac_ln_brk, MaxFieldSizePolicyConcept MaxFieldSize = MFS::no_trace, EmptyRowsPolicyConcept EmptyRows = ER::std_4180>
    class reader final : public std::conditional_t<!std::is_same_v<MaxFieldSize, MFS::no_trace>, MaxFieldSize, empty_t<void>>, protected std::conditional_t<!std::is_same_v<EmptyRows, ER::std_4180>, EmptyRows, empty_t<int>> {

        constexpr static std::size_t default_chunk_size = 1024 * 50;
        constexpr static bool max_field_size_no_trace = std::is_same_v<MaxFieldSize, MFS::no_trace>;
        constexpr static bool empty_rows_ignore = std::is_same_v<EmptyRows, ER::ignore>;

    public:
        /// User notification callback reason
        enum class notification_reason {
            max_field_size_reason = 0,
            empty_rows_reason
        };

    private:
        /// User notification callback type
        using notification_cb_t = std::function<void(notification_reason)>;

        template<typename T, typename G, typename... Bases>
        struct promise_type_base : public Bases ... {
            T mValue;

            auto yield_value(T value) {
                mValue = value;
                return std::suspend_always{};
            }

            G get_return_object() { return G{this}; };

            std::suspend_always initial_suspend() { return {}; }

            std::suspend_always final_suspend() noexcept { return {}; }

            void return_void() {}

            void unhandled_exception();

            static auto get_return_object_on_allocation_failure() {
                return G{nullptr};
            }
        };

        template<typename PT>
        struct coro_iterator {
            using coro_handle = std::coroutine_handle<PT>;

            coro_handle mCoroHdl{nullptr};
            bool mDone{true};

            using RetType = decltype(mCoroHdl.promise().mValue);

            void resume() {
                mCoroHdl.resume();
                mDone = mCoroHdl.done();
            }

            coro_iterator() = default;

            /* implicit */
            coro_iterator(coro_handle hco) : mCoroHdl{hco} {
                resume();
            }

            coro_iterator &operator++() {
                resume();
                return *this;
            }

            bool operator==(const coro_iterator &o) const { return mDone == o.mDone; }

            const RetType &operator*() const { return mCoroHdl.promise().mValue; }
        };

        template<typename T>
        struct awaitable_promise_type_base {
            std::optional<T> mRecentSignal;

            struct awaiter {
                std::optional<T> &mRecentSignal;

                [[nodiscard]] bool await_ready() const { return mRecentSignal.has_value(); }

                void await_suspend(std::coroutine_handle<>) {}

                T await_resume() {
                    assert(mRecentSignal.has_value());
                    auto tmp = *mRecentSignal;
                    mRecentSignal.reset();
                    return tmp;
                }
            };

            [[nodiscard]] awaiter await_transform(T) { return awaiter{mRecentSignal}; }
        };

        template<typename T, typename U>
        struct [[nodiscard]] async_generator {
            using promise_type = promise_type_base<T,
                    async_generator,
                    awaitable_promise_type_base<U>>;
            using PromiseTypeHandle = std::coroutine_handle<promise_type>;

            T operator()() {
                T tmp{};
                std::swap(tmp, mCoroHdl.promise().mValue);
                return tmp;
            }

            void send(U signal) {
                mCoroHdl.promise().mRecentSignal = signal;
                if (not mCoroHdl.done()) { mCoroHdl.resume(); }
            }

            async_generator(const async_generator &) = delete;

            async_generator(async_generator &&rhs) noexcept
                    : mCoroHdl{std::exchange(rhs.mCoroHdl, nullptr)} {}

            ~async_generator() {
                if (mCoroHdl) { mCoroHdl.destroy(); }
            }

        private:
            friend promise_type;

            explicit async_generator(promise_type *p) : mCoroHdl(PromiseTypeHandle::from_promise(*p)) {}

            PromiseTypeHandle mCoroHdl;
        };

        template<typename T>
        struct generator {
            using promise_type = promise_type_base<T, generator>;
            using PromiseTypeHandle = std::coroutine_handle<promise_type>;
            using iterator = coro_iterator<promise_type>;

            auto begin() -> iterator { return {mCoroHdl}; }

            auto end() -> iterator { return {}; }

            generator(generator const &) = delete;

            generator(generator &&rhs) noexcept: mCoroHdl(rhs.mCoroHdl) {
                rhs.mCoroHdl = nullptr;
            }

            ~generator() {
                if (mCoroHdl) { mCoroHdl.destroy(); }
            }

        private:
            friend promise_type;

            explicit generator(promise_type *p)
                    : mCoroHdl(PromiseTypeHandle::from_promise(*p)) {}

            PromiseTypeHandle mCoroHdl;
        };

        // Data chunk descriptor declaration
        struct data_chunk;

        template<typename C>
        using FSM_vector_cell_span = async_generator<C, data_chunk>;

    public:
        /// Cell span class forward declaration
        class cell_span;

    private:
        /// Header's field callback type of the span iteration mode
        using header_field_span_cb_t = std::function<void(cell_span &span)>;
        /// Value's field callback type of the span iteration mode 
        using value_field_span_cb_t = std::function<void(cell_span &span)>;
        /// New row callback type
        using new_row_cb_t = std::function<void()>;

    public:
        /// Row span class forward declaration
        struct row_span;
    private:
        /// Header row callback type
        using header_row_span_cb_t = std::function<void(row_span &span)>;
        /// Value row callback type   
        using value_row_span_cb_t = std::function<void(row_span &span)>;

    public:
        /// Row span class definition
        struct row_span : protected std::span<cell_span> {

            using std::span<cell_span>::front;
            using std::span<cell_span>::back;
            using std::span<cell_span>::begin;
            using std::span<cell_span>::end;
            using std::span<cell_span>::size;

            template<typename It>
            row_span(It first, size_t c) noexcept(noexcept(std::span<cell_span>(first, c))) : 
                std::span<cell_span>(first, c) {}

            /// Element access operator (by index).
            /// To be fast UB is allowed (cross-boundary access), should be noexcept.
            using std::span<cell_span>::operator[];

            /// Element access operator (by name).
            /// To be fast UB is allowed (cross-boundary access), should be noexcept.
            cell_span const &operator[](cell_string const & cs) const noexcept {
                auto const search = reader::col_name_2_index_map.find(cs);
                assert(search != reader::col_name_2_index_map.end());
                return std::span<cell_span>::operator[](search->second);
            }

            friend std::ostream &operator<<(std::ostream &os, row_span const &rs) {
                for (auto const &elem: rs)
                    os << elem.operator cell_string() << " ";
                return os;
            }
        };

        /// Cell/field span class definition
        class cell_span {
        private:
            typename cell_string::const_pointer b = nullptr;
            typename cell_string::const_pointer e = nullptr;

            friend class reader;

            [[nodiscard]] auto string() const {
                using namespace string_functions;
                // A mangled result string
                auto s = unquoted_cell_string{b, e};
                // If the field was (completely) quoted -> it must be unquoted
                unquote(s, Quote::value);
                // Fields partly quoted and not-quoted at all: must be spared from double quoting
                unique_quote(s, Quote::value);
                return s;
            }

        public:
            using reader_type = reader;

            cell_span() = default;
            /// Constructs cell span from pointers
            cell_span(cell_string::const_pointer b, cell_string::const_pointer e) noexcept: b(b), e(e) {}
            /// Constructs cell span from iterators
            cell_span(cell_string::const_iterator const& be, cell_string::const_iterator const &en) noexcept :
#if !defined(_MSC_VER)
                b(&*be), e(&*en) {}
#else
                b((be == en)? nullptr : &*be), e((be == en)? nullptr: b + std::distance(be,en)) {}
#endif
            /// Constructs cell_span from a string 
            explicit cell_span(cell_string const & s) noexcept :
                b(s.empty() ? nullptr : &*s.begin()), e(s.empty() ? nullptr : b + s.size()) {
            }

#if 0
            /// C-string strcmp()-like operation (1,0,-1). This is a quoted std::string comparison.
            [[nodiscard]] auto compare(cell_span const &other) const -> int {
                return cell_string(*this).compare(cell_string(other));
            }
#endif
            /* implicit conversion operator */
            /// Conversion to unquoted string
            operator unquoted_cell_string() const {
                return TrimPolicy::ret_trim(string());
            };

            /// Conversion to unchanged cell string except for applying trimming strategy
            operator cell_string() const {
                return TrimPolicy::ret_trim(raw_string());
            };

            /// Function that returns the completely unchanged cell string
            [[nodiscard]] cell_string raw_string() const {
                return {b, e};
            }

            /// Function that returns the completely unchanged cell string view 
            [[nodiscard]] std::string_view raw_string_view() const noexcept {
                return {b, e};
            }

            /// Conversion operator to any arithmetic type
            template<typename T>
            T as() const requires std::is_arithmetic_v<T> {
                auto const value = operator unquoted_cell_string();
                auto const first_not_backspace = value.find_first_not_of(' ');
                if (value.empty() or first_not_backspace == std::string::npos)
                    throw reader::exception("Argument is empty");
                else {
                    auto process_result = [&](auto && r, auto const & v) {
                        if (r.ec == std::errc())
                            return v;
                        else if (r.ec == std::errc::invalid_argument)
                            throw reader::exception("Argument isn't a number: ", value);
                        else if (r.ec == std::errc::result_out_of_range)
                            throw reader::exception("This number ", value, " is larger than ", type_name<std::decay_t<decltype(v)>>());
                        else
                            throw reader::exception("Unknown error with ", value);
                    };

                    assert(first_not_backspace != std::string::npos);
                    auto const value_data_offset = value.data() + first_not_backspace;
                    auto const value_data_end = value.data() + value.size();

                    if constexpr (!std::is_same_v<T, long double>) {
                        T v;
                        if constexpr (std::is_integral_v<T>) {
                            auto int_r = std::from_chars(value_data_offset, value_data_end, v);
                            return process_result(std::forward<std::from_chars_result>(int_r), v);
                        }
                        else { // float and double
#ifdef _MSC_VER
                            std::from_chars_result r;
                            r = std::from_chars(value_data_offset, value_data_end, v, std::chars_format::general);
                            return process_result(std::forward<std::from_chars_result>(r), v);
#else
                            fast_float::from_chars_result r;
                            r = fast_float::from_chars(value_data_offset, value_data_end, v);
                            return process_result(std::forward<fast_float::from_chars_result>(r), v);
#endif
                        }
                    } else { // long double
#ifdef _MSC_VER

                        T v;
                        return process_result(std::from_chars(value_data_offset, value_data_end, v, std::chars_format::general), v);
#else
                        // long doubles are not supported in either std or fast_float libraries' from_chars() functions
                        char *end;
                        T v{strtold(value.data(), &end)};

                        if (errno == ERANGE) {
                            errno = 0;
                            throw reader::exception("Range error, got inf: ", value);
                        } else if ((v == 0.0f) && (end == value.data()))
                            throw reader::exception("Argument isn't a number: ", value);
                        else 
                            return v;
#endif
                    }
                }
            }

            /// Compares the contents of this cell/field to something string-like
            bool operator==(std::string const &cs) const noexcept {
                auto const this_value = operator unquoted_cell_string();
                return std::equal(cs.begin(), cs.end(), this_value.begin(), this_value.end());
            }

            /// Compares the contents of this cell/field to a numeric value of T
            template<typename T>
            constexpr bool operator==(T const &other) const requires std::is_arithmetic_v<T> {
                return (other == as<T>());
            }

            /// Helpful for automatic precision evaluation
            template<typename T>
            constexpr T operator-(T const &other) const requires std::is_floating_point_v<T> {
                return std::abs(as<T>() - other);
            }

            /// Helper function. Note, it can highly depend on current trimming policy
            [[nodiscard]] bool empty() const {
                return TrimPolicy::ret_trim(string()).empty();
            }

            /// Helper cell size function
            [[nodiscard]] size_t size() const {
                return TrimPolicy::ret_trim(string()).size();
            }

        };

        /// Gets pointer to the beginning of the CSV data  
        char const * data() const noexcept {
            char const *sa = nullptr;
            std::visit([&](auto &&r) noexcept {
                sa = !src_offset ? ((r.size() >= 3 && r[0] == '\xef' && r[1] == '\xbb' && r[2] == '\xbf') ? &r[3] : &r[0]) : &r[0] + src_offset;
            }, src);
            return sa;
        }

        /// Gets the CSV size
        auto size() const noexcept {
            return get_finish_address() - data();
        }

    private:
#ifdef _MSC_VER
        static_assert(sizeof(cell_span) == 16);
#else
        static_assert(sizeof(cell_span) == 16);
#endif
        static_assert(std::is_move_constructible_v<cell_span>);
        static_assert(std::is_move_assignable<cell_span>::value);
        static_assert(std::is_copy_constructible<cell_span>::value);
        static_assert(std::is_copy_assignable<cell_span>::value);

        using coroutine_stream_value_type = mio::ro_mmap::value_type;
        using coroutine_stream_const_pointer_type = coroutine_stream_value_type const *;

        /// Offset between the beginning of the info and the beginning of the CSV source
        auto get_start_offset() const noexcept {
            std::size_t offset = 0;
            std::visit([&](auto &&arg) noexcept {
                offset = data() - &arg[0];
            }, src);
            return offset;
        }

        /// End address of the CSV source
        auto get_finish_address() const noexcept {
            char const * fa = nullptr;
            std::visit([&](auto const &r) noexcept {
                fa = std::addressof(r[0]) + r.size();
            }, src);
            return fa;
        }

        /// Data chunk descriptor definition
        struct data_chunk {
            coroutine_stream_value_type const *begin;
            coroutine_stream_value_type const *end;
        };

        /// Coroutine that sends every next data chunk
        template<typename Range, std::size_t RangeSize>
        auto data_chunk_sender(Range const &r) const noexcept -> generator<data_chunk> {
            data_chunk rs;
            decltype(rs.begin) const end_r = std::addressof(r[0]) + r.size();
            rs.begin = data();

            while (rs.begin < end_r) {
                rs.end = std::min(end_r, rs.begin + RangeSize);
                co_yield rs;
                rs.begin = rs.end;
            }
            assert(rs.begin >= end_r);
        }

#define control_max_field_size_macro               \
        if constexpr (!max_field_size_no_trace) {  \
            if (MaxFieldSize::reached()) {         \
                tracer.fire();                     \
            }                                      \
        }
#define reset_max_field_size_macro                 \
        if constexpr (!max_field_size_no_trace) {  \
            MaxFieldSize::reset();                 \
        }
#define control_empty_rows_macro                                     \
        if constexpr (empty_rows_ignore) {                           \
            EmptyRows::collect_recent(*rs.begin, LineBreak::value);  \
        }
#define check_empty_rows_macro(fn)                                 \
        if constexpr (empty_rows_ignore)                           \
        {                                                          \
            if (auto bytes = EmptyRows::skip(LineBreak::value)) {  \
                tracer.move_span_forward(bytes);                   \
                ++rs.begin;                                        \
            }                                                      \
            else tracer.fn(rs);                                    \
        } else

        /// Coroutine that parses CSV stream for all cases
        template <typename Range, typename T>
        auto data_chunk_parser(Range const & r, T && tracer) -> FSM_vector_cell_span< typename std::decay_t<T>::co_yield_type> {

            coroutine_stream_const_pointer_type end;
            coroutine_stream_value_type last_value;

            static std::vector<cell_span> tmp;

            tracer.initialize(data());
            end = std::addressof(r[0]) + r.size();
            last_value = r.back();

            for (;;) {
                auto rs = co_await data_chunk{};
                while (rs.begin != rs.end) {
                    control_empty_rows_macro
                    switch (*rs.begin) {
                        unsigned quote_counter;

                        case Delimiter::value:
                            tracer.on_delimiter(rs);
                            reset_max_field_size_macro
                            continue;

                        case Quote::value:
                            quote_counter = 1;
                            ++rs.begin;
                            control_max_field_size_macro
                            for (;;) {
                                while (rs.begin != rs.end) {
                                    control_empty_rows_macro
                                    // we now change "case-switch" to "if" to get rid of unavoidable "goto".
                                    if (*rs.begin == Delimiter::value) {
                                        if (!(quote_counter & 1)) {
                                            tracer.on_delimiter(rs);
                                            reset_max_field_size_macro
                                            break;
                                        }
                                        ++rs.begin;
                                        control_max_field_size_macro
                                    } else if (*rs.begin == LineBreak::value) {
                                        if (!(quote_counter & 1)) {
                                            check_empty_rows_macro(on_lf)
                                            tracer.on_lf(rs);
                                            reset_max_field_size_macro
                                            break;
                                        }
                                        ++rs.begin;
                                        control_max_field_size_macro
                                    } else {
                                        [[likely]]
                                        quote_counter += (Quote::value == *rs.begin++) ? 1 : 0; // branch-less
                                        control_max_field_size_macro
                                    }
                                }
                                if (rs.begin == rs.end) {
                                    [[unlikely]]
                                    // TODO: could you get rid of each-time-checking that for completely sane CSV file
                                    //  so that compiler's optimizer would generate a faster code? Don't think so.
                                    if (rs.end == end && last_value != LineBreak::value) {
                                        check_empty_rows_macro(on_missed_lf)
                                        tracer.on_missed_lf(rs);
                                        reset_max_field_size_macro
                                    }

                                    co_yield (tracer.co_yield_thing());
                                    tracer.cleanup_chunk_artifacts();
                                    rs = co_await data_chunk{};
                                } else {
                                    [[likely]]
                                    break;
                                }
                            }
                            continue;

                        case LineBreak::value:
                            check_empty_rows_macro(on_lf)
                            tracer.on_lf(rs);
                            reset_max_field_size_macro
                            continue;

                        default:
                            ++rs.begin;
                            control_max_field_size_macro
                            continue;
                    }
                }

                if (rs.end == end && last_value != LineBreak::value) {
                    [[unlikely]];
                    check_empty_rows_macro(on_missed_lf)
                    tracer.on_missed_lf(rs);
                    reset_max_field_size_macro
                }

                co_yield (tracer.co_yield_thing());
                tracer.cleanup_chunk_artifacts();
            }
        }

#undef control_max_field_size_macro
#undef reset_max_field_size_macro
#undef control_empty_rows_macro
#undef check_empty_rows_macro

        struct v_field_span_caller_tag;
        struct hv_field_span_caller_tag;

        template<class T>
        struct max_field_size_tracer {
        private:
            mutable bool value_ = false;
        public:
            inline void fire() noexcept {
                value_ = true;
            }
            inline auto mfs_fired() const noexcept {
                auto tmp = value_;
                value_ = false;
                return tmp;
            }
        };

        template<class T>
        struct ignore_empty_rows_tracer {
            mutable bool value_ = false;
            inline void move_span_forward(unsigned bytes) noexcept {
                if constexpr(has_member(T,span)) {
                    static_cast<T *>(this)->span.b += bytes;
                    if (!value_)
                        value_ = true;
                }
            }
            inline auto er_fired() const noexcept {
                auto tmp = value_;
                value_ = false;
                return tmp;
            }
        };

        void fire_notification_handler(auto const & tracer) const {
            if constexpr (!max_field_size_no_trace) {
                if (notification_cb && tracer.mfs_fired())
                    notification_cb(notification_reason::max_field_size_reason);
            }
            if constexpr (empty_rows_ignore) {
                if (notification_cb && tracer.er_fired())
                    notification_cb(notification_reason::empty_rows_reason);
            }
        }

        /// Implementation of the span iteration mode
        template <typename SpanCallerTag, std::size_t ParseChunkSize, template <class> class ... Bases>
        void run_spans_impl()  {

            /// Specific methods implementer for parsing coroutine
            class tracer : public Bases<tracer>... {
            protected:
                std::vector<cell_span> vec;
            public:
                cell_span span; // fixme : why public?
                using co_yield_type = std::add_pointer_t<std::vector<cell_span>>;

                inline void initialize(char const* ptr) noexcept { vec.clear(); span.b = ptr;}
                inline void on_delimiter (data_chunk & rs) {
                    vec.emplace_back(span.b, rs.begin);
                    span.b = ++rs.begin;
                }
                inline void on_lf (data_chunk & rs) {
                    vec.emplace_back(span.b, rs.begin);
                    span.b = ++rs.begin;
                }
                inline void on_missed_lf (data_chunk & rs) {
                    vec.emplace_back(span.b, span.b + (rs.end - span.b));
                }
                inline co_yield_type co_yield_thing() noexcept { return &vec; }
                inline void cleanup_chunk_artifacts() noexcept { vec.clear(); }
            };

            tracer t;

            std::visit([&](auto&& arg) {
                unsigned cls;
                if constexpr (std::is_same_v<SpanCallerTag, hv_field_span_caller_tag>)
                    cls = cols<ParseChunkSize>();

                auto source = data_chunk_sender<decltype(arg), ParseChunkSize>(arg);
                auto p = data_chunk_parser(arg, t);

                std::function<void(cell_span & span)> hfs_or_vfs_cb = [&cls, this, &hfs_or_vfs_cb] (auto & elem) {
                    hfs_cb(elem);
                    if (!--cls)
                        hfs_or_vfs_cb = vfs_cb;
                };

                for (auto const & b: source) {
                    p.send(b);

                    fire_notification_handler(t);

                    auto && v = *p();
                    for (auto && elem : v) {
                        char is_lf;
                        if constexpr (std::is_same_v<SpanCallerTag, v_field_span_caller_tag>)
                            vfs_cb((is_lf = *elem.e, elem));
                        else if constexpr (std::is_same_v<SpanCallerTag, hv_field_span_caller_tag>)
                            hfs_or_vfs_cb((is_lf = *elem.e, elem));

                        if (is_lf == LineBreak::value)
                            new_row_cb();
                    }
                }

                if (arg.back() != LineBreak::value)
                    new_row_cb();

            }, src);
        }

        // TODO: make a local lambda
        void iterate(auto span) const {
            unsigned index = 0;
            for (auto const & elem : span)
                col_name_2_index_map[elem.operator cell_string()] = index++;
        }

        class v_row_span_caller_tag;
        class hv_row_span_caller_tag;

        /// Implementation of the rows iteration mode
        template <typename Tag, std::size_t ParseChunkSize, template<class> class ... Bases>
        void run_rows_impl(auto cols) {

            /// Specific methods implementer for parsing coroutine
            class tracer : public Bases<tracer>... {
            public:
                cell_span span;
            private:
                std::vector<cell_span> vec;
                size_t cols;
            public:
                using co_yield_type = decltype(&vec);

                explicit tracer (std::size_t cols) : cols (cols) {}

                inline void initialize(char const* ptr) noexcept { vec.clear(); span.b = ptr; }
                inline void on_delimiter (data_chunk & rs) {
                    vec.emplace_back(span.b, rs.begin);
                    span.b = ++rs.begin;
                }
                inline void on_lf (data_chunk & rs) {
                    vec.emplace_back(span.b, rs.begin);
                    span.b = ++rs.begin;
                }
                inline void on_missed_lf (data_chunk & rs) {
                    vec.emplace_back(span.b, span.b + (rs.end - span.b));
                }
                inline co_yield_type co_yield_thing() { return &vec; }
                inline void cleanup_chunk_artifacts() {
                    if (auto const elems_to_delete = vec.size()/cols * cols)
                        vec.erase(vec.begin(), vec.begin() + elems_to_delete);
                }
            };

            tracer t (cols);

            std::function<void(row_span & r)> hrs_or_vrs_cb = [this, &hrs_or_vrs_cb] (auto & span) {
                iterate(span);
                hrs_cb(span);
                hrs_or_vrs_cb = vrs_cb;
            };

            std::visit([&](auto&& arg) {
                if (!cols)
                    throw implementation_exception("An incorrect assumption, columns number is zero.");

                auto source = data_chunk_sender<decltype(arg), ParseChunkSize>(arg);
                auto p = data_chunk_parser(arg, t);

                for (auto const & b: source) {
                    p.send(b);

                    fire_notification_handler(t);

                    auto && v = *p();
                    if (auto const rows = v.size()/cols) {
                        for (auto row = 0u; row != rows; ++row) {
                            row_span span (v.begin()+cols*row, cols);
                            if constexpr (std::is_same_v<Tag, v_row_span_caller_tag>)
                                vrs_cb(span);
                            else if constexpr (std::is_same_v<Tag, hv_row_span_caller_tag>)
                                hrs_or_vrs_cb(span);
                        }
                    }
                }
            }, src);
        }

        /// Implementation of the columns getter
        template <std::size_t ParseChunkSize = default_chunk_size, template<class> class ... Bases>
        [[nodiscard]] auto cols_impl() noexcept (max_field_size_no_trace) -> std::size_t {

            /// Specific methods implementer for parsing coroutine
            class tracer : public Bases<tracer>... {
                std::size_t cols;
                std::optional<std::size_t> proxy_cols;
                static_assert(std::is_same_v<decltype(proxy_cols), std::optional<std::size_t>>);
                bool done;
            public:
                tracer() : cols{0}, done{false} {}
                using co_yield_type = decltype(proxy_cols);
                inline void initialize(char const * const) noexcept {proxy_cols = std::nullopt; }
                inline void on_delimiter (data_chunk & rs) noexcept { rs.begin++; cols = done ? cols : cols + 1; }
                inline void on_lf (data_chunk & rs) noexcept { rs.begin++; on_missed_lf(rs); }
                inline void on_missed_lf (data_chunk&) noexcept { cols = done ? cols : cols + 1; done = true; }
                inline auto & co_yield_thing() noexcept { if (done) { proxy_cols = cols; } return proxy_cols; }
                inline void cleanup_chunk_artifacts() noexcept { }

            };

            tracer t;

            auto cols {0};
            std::visit([&](auto&& arg) noexcept (max_field_size_no_trace) {
                auto source = data_chunk_sender<decltype(arg), ParseChunkSize>(arg);
                auto p = data_chunk_parser(arg, t);
                for (auto const & b: source) {
                    p.send(b);
                    auto && v = p();

                    fire_notification_handler(t);

                    if (v.has_value()) {
                        cols = v.value();
                        return;
                    }
                }
            }, src);
            return cols;
        }

        /// Implementation of the rows getter
        template <std::size_t ParseChunkSize=default_chunk_size, template <class> class ... Bases>
        [[nodiscard]] auto rows_impl() noexcept -> std::size_t {

            /// Specific methods implementer for parsing coroutine
            class tracer : public Bases<tracer>... {
                size_t lines;
                std::optional<std::size_t> proxy_lines;
            public:
                tracer() : lines(0) {}
                using co_yield_type = decltype(proxy_lines);
                inline void initialize(char const * const) noexcept {}
                inline void on_delimiter (data_chunk & rs) noexcept { rs.begin++; }
                inline void on_lf (data_chunk & rs) noexcept { rs.begin++; lines++; }
                inline void on_missed_lf (data_chunk&) noexcept { lines++; }
                inline auto & co_yield_thing() noexcept { if (lines) { proxy_lines = lines; lines = 0; } return proxy_lines; }
                inline void cleanup_chunk_artifacts() noexcept { if (proxy_lines.has_value()) proxy_lines = std::nullopt; }
            };

            tracer t;

            auto rows {0};
            std::visit([&](auto&& arg) noexcept {
                auto source = data_chunk_sender<decltype(arg), ParseChunkSize>(arg);
                auto p = data_chunk_parser(arg, t);
                for (auto const & b: source) {
                    p.send(b);
                    auto && v = p();

                    fire_notification_handler(t);

                    if (v.has_value())
                        rows += v.value();
                }
            }, src);
            return rows;
        }

        /// Implementation of the validation
        template <std::size_t ParseChunkSize=default_chunk_size, template<class> class ... Bases>
        [[nodiscard]] auto validate_impl() -> reader& {

            /// Specific methods implementer for parsing coroutine
            class tracer : public Bases<tracer>... {
                std::size_t cols = 0;
                std::vector<std::size_t> cols_vector;
            public:
                using co_yield_type = decltype(&cols_vector);
                inline void initialize(char const * const) noexcept { cols = 0;}
                inline void on_delimiter (data_chunk & rs) noexcept { rs.begin++; cols++;}
                inline void on_lf (data_chunk & rs) { rs.begin++; cols_vector.push_back(++cols); cols = 0; }
                inline void on_missed_lf (data_chunk&) { cols_vector.push_back(++cols); cols = 0; }
                inline co_yield_type co_yield_thing() noexcept { return &cols_vector; }
                inline void cleanup_chunk_artifacts() { cols_vector.clear(); }
            };

            tracer t;

            std::size_t orig_cols = {0u}; /*CLion: The value is never user. It is not true!*/
            std::size_t rows_accumulator = {1u}; /*CLion: The value is never user. It is not true!*/

            using vector_of_columns = std::vector<std::size_t>;

            std::function<void(vector_of_columns const & v)> state_m;

            std::ostringstream validated_errors;
            std::string prefix;

            std::function<void(vector_of_columns const & v)> state_2 = [&] (auto & v)  {
                if (!v.empty()) {
                    auto it = v.begin();
                    while (it != v.end()) {
                        it = std::find_if_not(it, v.end(), [&](auto &elem) {return orig_cols == elem;});
                        if (it != v.end()) {
                            validated_errors << prefix << '\"' << "Line " << rows_accumulator + std::distance(v.begin(), it) << ": Expected " << orig_cols << " columns, found " << *it << " columns" << '\"';
                            prefix = ',';
                            ++it;
                        }
                    }
                    rows_accumulator += v.size();
                }
            };

            std::function<void(vector_of_columns const & v)> state_1 = [&] (auto & v) noexcept {
                if (!v.empty()) {
                    orig_cols = v[0];
                    state_2(v);
                    state_m = state_2;
                    return;
                }
                assert(state_2);
                state_2(v);
            };

            std::visit([&](auto&& arg) {
                auto source = data_chunk_sender<decltype(arg), ParseChunkSize>(arg);
                auto p = data_chunk_parser(arg, t);
                state_m = state_1;

                for (auto const & b: source) {
                    p.send(b);
                    auto && v = *p();
                    fire_notification_handler(t);
                    state_m(v);
                }

                if (validated_errors.tellp())
                    throw exception(validated_errors.str());

                if (!orig_cols)
                    throw exception ("Use of \"move from\" state object");

                validated_shape = validated_shape_t(rows_accumulator - 1, orig_cols);
            }, src);

            return *this;
        }

        /// Implementation of the skip rows facility
        template <std::size_t ParseChunkSize=default_chunk_size, template <class> class ... Bases>
        [[nodiscard]] auto skip_rows_impl(std::size_t rows_to_skip, std::size_t offset) noexcept -> std::size_t {

            /// Specific methods implementer for parsing coroutine
            class tracer : public Bases<tracer>... {
                std::size_t rows_to_skip_;
                std::optional<std::size_t> proxy_src_offset;
                std::size_t byte_accumulator;
                cell_string::const_pointer b; // initialized in initialize()
                bool done;
            public:
                explicit tracer(std::size_t rows_to_skip, std::size_t off) :
                    rows_to_skip_{rows_to_skip}, byte_accumulator{off}, done{false} {}
                using co_yield_type = decltype(proxy_src_offset);
                inline void initialize(char const * const ptr) noexcept { b = ptr;}
                inline void on_delimiter (data_chunk & rs) {
                    byte_accumulator += done ? 0 : rs.begin - b + 1;
                    b = ++rs.begin;
                }
                inline void on_lf (data_chunk & rs) noexcept {
                    on_delimiter(rs);
                    if (!--rows_to_skip_)
                        done = true;
                }
                inline void on_missed_lf (data_chunk & rs) noexcept {
                    on_lf(rs);
                }
                inline auto & co_yield_thing() noexcept {
                    if (done) { proxy_src_offset = byte_accumulator; } return proxy_src_offset;
                }
                inline void cleanup_chunk_artifacts() noexcept {
                    if (proxy_src_offset.has_value()) proxy_src_offset = std::nullopt;
                }

            };

            tracer t (rows_to_skip, offset);

            auto new_offset {0u};
            std::visit([&](auto&& arg) noexcept {
                auto source = data_chunk_sender<decltype(arg), ParseChunkSize>(arg);
                auto p = data_chunk_parser(arg, t);
                for (auto const & b: source) {
                    p.send(b);
                    auto && v = p();
                    if (v.has_value()) {
                        new_offset = v.value();
                        return;
                    }
                }
            }, src);
            return new_offset;
        }

        // Multi-source CSV
        std::variant<mio::ro_mmap, std::string, std::vector<char>> src;

        using validated_shape_t = std::pair<std::size_t, std::size_t>;
        std::optional<validated_shape_t> validated_shape = std::nullopt;

        size_t src_offset = 0;

        mutable new_row_cb_t new_row_cb;

        mutable header_field_span_cb_t hfs_cb;
        mutable value_field_span_cb_t vfs_cb;

        mutable header_row_span_cb_t hrs_cb;
        mutable value_row_span_cb_t vrs_cb;
        mutable notification_cb_t notification_cb;

        inline static std::unordered_map<cell_string, unsigned> col_name_2_index_map;
        static constexpr char const * const arg_is_empty = "Argument can not be empty.";
    public:
        using trim_policy_type = TrimPolicy;
        using quote_type = Quote;
        using delimiter_type = Delimiter;
        using line_break_type = LineBreak;

        static_assert(delimiter_type::value != quote_type::value);
        static_assert(line_break_type::value != quote_type::value);
        static_assert(line_break_type::value != delimiter_type::value);

        static_assert (trim_chars_do_not_conflict_with(delimiter_type::value, std::is_same_v<TrimPolicy, trim_policy::trimming<trim_policy::chars>>));
        static_assert (trim_chars_do_not_conflict_with(quote_type::value, std::is_same_v<TrimPolicy, trim_policy::trimming<trim_policy::chars>>));
        static_assert (trim_chars_do_not_conflict_with(line_break_type::value, std::is_same_v<TrimPolicy, trim_policy::trimming<trim_policy::chars>>));

        // TODO: stop calling for rvalue string...
        /// Constructs reader from disk file source
        explicit reader(std::filesystem::path const & csv_src) : src {mio::ro_mmap {}} {
            auto const str = csv_src.string();
            if (str.find(".gz") == std::string::npos) {
                std::error_code mmap_error;
                std::get<0>(src).map(str.c_str(), mmap_error);
                if (mmap_error)
                    throw exception (mmap_error.message(), " : ", str );
            } else
                src = EzGz::IGzFile<>(str).readAll();
        }

        /// Constructs reader from string source
        explicit reader (std::string csv_src) : src {std::move(csv_src)} {
            if (std::get<1>(src).empty())
                throw exception (arg_is_empty);
        }

        /// Let us express C-style string constructor via usual std::string constructor
        explicit reader (const char * csv_src) : reader(cell_string(csv_src)) {}

        //TODO: generalize via concept [][] MatrixCont std::vector<std::vector<std::string>>

        /// Constructs reader from some form of matrix...
        template <typename MatrixContainer>
        explicit reader (MatrixContainer const & m) : src {std::string{}} {
            auto & ref = std::get<1>(src);
            for (auto const & e : m) {
                for_each(e.begin(), e.end() - 1, [&](auto const & elem) { ref += elem; ref += ','; });
                ref += e.back();
                ref += LineBreak::value;
            }
            if (ref.empty())
                throw exception (arg_is_empty);
        }

        // Move operations work by default - see unit tests
        reader (reader && other) noexcept = default;
        auto operator=(reader && other) noexcept -> reader & = default;

        /// Notification handler installer 
        auto & install_notification_handler(notification_cb_t cb) {
            notification_cb = std::move(cb);
            return *this;
        }

        // TODO: is const?
        /// Columns getter
        template <std::size_t ParseChunkSize=default_chunk_size>
        [[nodiscard]] auto cols() noexcept (max_field_size_no_trace) -> std::size_t {
            if constexpr (!max_field_size_no_trace && empty_rows_ignore)
                return cols_impl<ParseChunkSize, max_field_size_tracer, ignore_empty_rows_tracer>();
            else if constexpr (max_field_size_no_trace && empty_rows_ignore)
                return cols_impl<ParseChunkSize, empty_t, ignore_empty_rows_tracer>();
            else if constexpr (!max_field_size_no_trace && !empty_rows_ignore)
                return cols_impl<ParseChunkSize, max_field_size_tracer, empty_t>();
            else
                return cols_impl<ParseChunkSize, empty_t>();
        }

        /// Rows getter
        template <std::size_t ParseChunkSize=default_chunk_size>
        [[nodiscard]] auto rows() noexcept -> std::size_t requires (std::is_same_v<EmptyRows, ER::std_4180>) {
            if constexpr (!max_field_size_no_trace && empty_rows_ignore)
                return rows_impl<ParseChunkSize, max_field_size_tracer, ignore_empty_rows_tracer>();
            else if constexpr (max_field_size_no_trace && empty_rows_ignore)
                return rows_impl<ParseChunkSize, empty_t, ignore_empty_rows_tracer>();
            else if constexpr (!max_field_size_no_trace && !empty_rows_ignore)
                return rows_impl<ParseChunkSize, max_field_size_tracer, empty_t>();
            else
                return rows_impl<ParseChunkSize, empty_t>();
        }

        /// Executes CSV-stream validation;)
        template <std::size_t ParseChunkSize=default_chunk_size>
        [[nodiscard]] auto validate() -> reader& {
            if constexpr (!max_field_size_no_trace && empty_rows_ignore)
                return validate_impl<ParseChunkSize, max_field_size_tracer, ignore_empty_rows_tracer>();
            else if constexpr (max_field_size_no_trace && empty_rows_ignore)
                return validate_impl<ParseChunkSize, empty_t, ignore_empty_rows_tracer>();
            else if constexpr (!max_field_size_no_trace && !empty_rows_ignore)
                return validate_impl<ParseChunkSize, max_field_size_tracer, empty_t>();
            else
                return validate_impl<ParseChunkSize, empty_t>();
#if 0
            return validate_impl<ParseChunkSize
                , std::conditional_t<!max_field_size_no_trace, max_field_size_tracer, empty_t>
                , std::conditional_t<empty_rows_ignore, ignore_empty_rows_tracer, empty_t>>();
#endif
        }

        /// Validated rows getter
        [[nodiscard]] auto validated_rows() const -> std::size_t {
            if (validated_shape.has_value())
                return validated_shape.value().first;
            else
                throw exception ("There are no ready rows. Call validate() method first.");
        }

        /// Validated cols getter
        [[nodiscard]] auto validated_cols() const -> std::size_t {
            if (validated_shape.has_value())
                return validated_shape.value().second;
            else
                throw exception ("There are no ready cols. Call validate() method first.");
        }

        /// Executes span iteration mode
        template <std::size_t ParseChunkSize=default_chunk_size>
        void run_spans(value_field_span_cb_t v, new_row_cb_t n=[]{}) {
            vfs_cb = std::move(v);
            new_row_cb = std::move(n);

            if constexpr (!max_field_size_no_trace && empty_rows_ignore)
                run_spans_impl<v_field_span_caller_tag, ParseChunkSize, max_field_size_tracer,ignore_empty_rows_tracer>();
            else if constexpr (max_field_size_no_trace && empty_rows_ignore)
                run_spans_impl<v_field_span_caller_tag, ParseChunkSize, empty_t,ignore_empty_rows_tracer>();
            else if constexpr (!max_field_size_no_trace && !empty_rows_ignore)
                run_spans_impl<v_field_span_caller_tag, ParseChunkSize, max_field_size_tracer,empty_t>();
            else
                run_spans_impl<v_field_span_caller_tag, ParseChunkSize, empty_t>();
#if 0
            run_spans_impl<v_field_span_caller_tag, ParseChunkSize
                , my_conditional2<!max_field_size_no_trace, max_field_size_tracer, empty_t>::type>();
#endif
        }

        /// Executes Spanning mode (overload)
        template <std::size_t ParseChunkSize=default_chunk_size>
        void run_spans(header_field_span_cb_t h, value_field_span_cb_t v, new_row_cb_t n=[]{}) {
            hfs_cb = std::move(h);
            vfs_cb = std::move(v);
            new_row_cb = std::move(n);
            if constexpr (!max_field_size_no_trace && empty_rows_ignore)
                run_spans_impl<hv_field_span_caller_tag, ParseChunkSize, max_field_size_tracer,ignore_empty_rows_tracer>();
            else if constexpr (max_field_size_no_trace && empty_rows_ignore)
                run_spans_impl<hv_field_span_caller_tag, ParseChunkSize, empty_t,ignore_empty_rows_tracer>();
            else if constexpr (!max_field_size_no_trace && !empty_rows_ignore)
                run_spans_impl<hv_field_span_caller_tag, ParseChunkSize, max_field_size_tracer,empty_t>();
            else
                run_spans_impl<hv_field_span_caller_tag, ParseChunkSize, empty_t>();
        }

        /// Executes row iteration mode
        template <std::size_t ParseChunkSize=default_chunk_size>
        void run_rows(value_row_span_cb_t v) {
            vrs_cb = std::move(v);
            if constexpr (!max_field_size_no_trace && empty_rows_ignore)
                run_rows_impl<v_row_span_caller_tag, ParseChunkSize, max_field_size_tracer, ignore_empty_rows_tracer>(cols<ParseChunkSize>());
            else if constexpr (max_field_size_no_trace && empty_rows_ignore)
                run_rows_impl<v_row_span_caller_tag, ParseChunkSize, empty_t, ignore_empty_rows_tracer>(cols<ParseChunkSize>());
            else if constexpr (!max_field_size_no_trace && !empty_rows_ignore)
                run_rows_impl<v_row_span_caller_tag, ParseChunkSize, max_field_size_tracer, empty_t>(cols<ParseChunkSize>());
            else
                run_rows_impl<v_row_span_caller_tag, ParseChunkSize, empty_t>(cols<ParseChunkSize>());
        }

        //Executes Row iteration mode (overload)
        template <std::size_t ParseChunkSize=default_chunk_size>
        void run_rows(header_row_span_cb_t h, value_row_span_cb_t v) {
            hrs_cb = std::move(h);
            vrs_cb = std::move(v);
            if constexpr (!max_field_size_no_trace && empty_rows_ignore)
                run_rows_impl<hv_row_span_caller_tag, ParseChunkSize, max_field_size_tracer, ignore_empty_rows_tracer>(cols<ParseChunkSize>());
            else if constexpr (max_field_size_no_trace && empty_rows_ignore)
                run_rows_impl<hv_row_span_caller_tag, ParseChunkSize, empty_t, ignore_empty_rows_tracer>(cols<ParseChunkSize>());
            else if constexpr (!max_field_size_no_trace && !empty_rows_ignore)
                run_rows_impl<hv_row_span_caller_tag, ParseChunkSize, max_field_size_tracer, empty_t>(cols<ParseChunkSize>());
            else
                run_rows_impl<hv_row_span_caller_tag, ParseChunkSize, empty_t>(cols<ParseChunkSize>());
        }

        template <std::size_t ParseChunkSize = default_chunk_size>
        auto skip_rows(std::size_t rows_to_skip) -> reader& {
            if constexpr (!max_field_size_no_trace && empty_rows_ignore)
                src_offset = rows_to_skip ? skip_rows_impl<ParseChunkSize, max_field_size_tracer, ignore_empty_rows_tracer>(rows_to_skip, get_start_offset()) : 0;
            else if constexpr (max_field_size_no_trace && empty_rows_ignore)
                src_offset = rows_to_skip ? skip_rows_impl<ParseChunkSize, empty_t,ignore_empty_rows_tracer>(rows_to_skip, get_start_offset()) : 0;
            else if constexpr (!max_field_size_no_trace && !empty_rows_ignore)
                src_offset = rows_to_skip ? skip_rows_impl<ParseChunkSize, max_field_size_tracer, empty_t>(rows_to_skip, get_start_offset()) : 0;
            else 
                src_offset = rows_to_skip ? skip_rows_impl<ParseChunkSize, empty_t>(rows_to_skip, get_start_offset()) : 0;
            return *this;
        }

        /// Main exception class
        struct exception : std::runtime_error {
            template <typename ... Types>
            explicit constexpr exception(Types ... args) : std::runtime_error("") {
                save_details(args...);
            }

            [[nodiscard]] constexpr auto what() const noexcept -> char const* override {
                return msg.c_str();
            }

        private:
            std::string msg;

            void save_details() {}

            template <class First, class ... Rest>
            void save_details(First && first, Rest && ... rest) {
                save_detail(std::forward<First>(first));
                save_details(std::forward<Rest>(rest)...);
            }

            template <typename T>
            void save_detail(T && v) {
                if constexpr(std::is_arithmetic_v<std::decay_t<T>>) 
                    msg += std::to_string(v); 
                else 
                    msg += v;
            }
        };

        /// Auxiliary exception class
        struct implementation_exception : exception {
            template <typename ... Types>
            explicit implementation_exception(Types ... tt ) : exception(tt...) {}
        };

        /// Obtains header cell range
        template <typename Container=std::vector<cell_span>, std::size_t ParseChunkSize=default_chunk_size>
        Container header() requires has_emplace_back<Container> {
            Container c;
            try {
                run_rows<ParseChunkSize>([&c](auto span) {
                    for (auto & elem: span)
                        c.emplace_back(elem);
                    throw implementation_exception();
                });
            }
            catch (implementation_exception const &) {}
            catch (exception const &) {
                throw;
            }
            return c;
        }

        /// Seek to the beginning
        template <std::size_t ParseChunkSize=default_chunk_size>
        void seek() requires (std::is_same_v<EmptyRows, ER::std_4180>) {
            auto rows_to_skip = 0u;
            try {
                run_rows<ParseChunkSize>([&](auto & span) {
                    if (span.size() == 1 and span.front().empty())
                        rows_to_skip++;
                    else
                        throw implementation_exception();
                });
            }
            catch (implementation_exception const &) {}
            if (rows_to_skip)
                skip_rows<ParseChunkSize>(rows_to_skip);
        }

        /// Bridge from cell_span to the csv kit cell span with type support 
        template <bool Unquoted>
        class typed_span : public cell_span {
        private:
            mutable long double value = 0;                                    /**< Cached numeric value */
            mutable vince_csv::DataType type_ = vince_csv::DataType::UNKNOWN; /**< Cached data type value */
            mutable bool rebind_conversion {false};
            mutable unsigned char prec = 0;
#if defined(_MSC_VER)
            mutable bool reserved[2] {};
#else
            mutable bool reserved[10] {};
#endif
            void get_value() const;
            using cell_span::b;
            using cell_span::e;

        public:
            using class_type = typed_span<Unquoted>;
            using reader_type = reader;

            /// What can be migrated may be migrated
            using cell_span::operator unquoted_cell_string;
            using cell_span::operator cell_string;
            using cell_span::raw_string;
            using cell_span::raw_string_view;

            /// Construction by defaault
            typed_span() = default;

            /// Construction from the cell_span
            explicit typed_span(cell_span const &cs);
            /// Construction from string
            explicit typed_span(std::string const & s);

            /// Assignment from the cell_span
            typed_span &operator=(cell_span const &) noexcept;
            static constexpr bool is_unquoted() noexcept {
                return Unquoted;
            }
            static constexpr bool is_quoted() noexcept {
                return !Unquoted;
            }

            /// Comparison of two typeaware cells
            [[nodiscard]] auto compare(typed_span const &other) const -> int;

            template <bool U>
            struct rebind {
                using other = typed_span<U>;
            };

            /// Convert typed_span<false> to typed_span<true>, reset data type only once!
            operator typed_span<!Unquoted> const & () const {
                if (!rebind_conversion) {
                    type_ = vince_csv::DataType::UNKNOWN;
                    rebind_conversion = true;
                }
                return reinterpret_cast<typed_span<!Unquoted> const &>(*this);
            }

            friend std::ostream& operator<< (std::ostream& os, typed_span const& cs) {
                if constexpr (!Unquoted)
                    os << cs.operator cell_string();
                else
                    os << cs.operator unquoted_cell_string();
                return os;
            }

            /// Trying to do C locale representaion
            inline bool to_C_locale(std::string & s) const;

            /// Obtains numeric value of it
            long double num() const;

            /// Knowing it is bool, obtains bool value
            inline bool unsafe_bool() const noexcept;

            /// Returns true if it is an empty string or a string of whitespace characters
            constexpr bool is_nil() const;

            /// Check if it is any null rep.
            inline bool is_null() const;

            /// Returns true if it is a non-numeric, non-empty string
            constexpr bool is_str() const;

            /// Returns true if it is an integer or float
            constexpr bool is_num() const;

            /// Return true if it is boolean
            bool is_boolean() const;

            /// Returns true if field is an integer
            constexpr bool is_int() const;

            /// Returns true if field is a floating point value
            constexpr bool is_float() const;

            /// Returns the type of the underlying CSV data 
            constexpr vince_csv::DataType type() const;

            /// Returns cell size in utf8 characters
            constexpr unsigned str_size_in_symbols() const;

            /// Unsafely returns cell size in utf8 characters
            constexpr unsigned unsafe_str_size_in_symbols() const;

            /// Returns string representation
            constexpr auto str() const;

            /// Returns std::tuple<bool, date::sys_seconds> representaion for datetimes 
            auto datetime(std::string const & extra_dt_fmt="") const;

            /// Returns std::tuple<bool, date::sys_seconds> representaion for dates
            auto date(std::string const & extra_date_fmt="") const;

            /// Returns std::tuple<bool, std::string> representaion for timedeltas
            auto timedelta_tuple() const;

            /// Returns this timedelta value in seconds
            long double timedelta_seconds() const;

            /// Imbues this numberic locale to numeric cells
            static void imbue_num_locale(std::locale & num_loc) noexcept;

            /// Numeric locale getter
            static std::locale & num_locale();

            /// States whether to calculate most decimal places for some utils
            static void maxprecision_flag(bool) noexcept;

            /// Cell numeric precision getter
            unsigned char precision() const noexcept;
        private:
            static inline auto date_datetime_call_operator_implementation(auto const & ccs, auto & formats);

            struct datetime_format_proc {
                explicit datetime_format_proc(std::string const & extra_dt_fmt);
                auto operator()(typed_span const & ccs) const;
            private:
                mutable std::vector<std::string> formats;
            };

            struct date_format_proc {
                explicit date_format_proc (std::string const & extra_date_fmt);
                auto operator()(typed_span const & ccs) const;
            private:
                mutable std::vector<std::string> formats;
            };

            inline static std::locale * num_locale_ {nullptr};
            inline static bool no_maxprecision {false};
            inline unsigned char get_precision(std::string & rep) const;
            inline bool can_be_money(std::string const & s) const;
            inline void get_num_from_money() const;
        public:
            enum class date_parser_backend_t {
                compiler_supported = 0,
                date_lib_supported 
            };
        private:
            inline static date_parser_backend_t date_parser_backend {date_parser_backend_t::compiler_supported};
        public:
            static void setup_date_parser_backend(date_parser_backend_t) noexcept;
        };

        static_assert(std::is_move_constructible_v<typed_span<true>>);
        static_assert(std::is_move_assignable_v<typed_span<true>>);
        static_assert(std::is_copy_constructible_v<typed_span<true>>);
        static_assert(std::is_copy_assignable_v<typed_span<true>>);
        static_assert(std::is_move_constructible_v<typed_span<false>>);
        static_assert(std::is_move_assignable_v<typed_span<false>>);
        static_assert(std::is_copy_constructible_v<typed_span<false>>);
        static_assert(std::is_copy_assignable_v<typed_span<false>>);
        static_assert(sizeof(typed_span<false>) == sizeof(typed_span<true>));
#if defined(_MSC_VER)
        static_assert(sizeof(typed_span<false>) == 32);
        static_assert(sizeof(long double) == 8);
#else
        static_assert(sizeof(typed_span<false>) == 48);
        static_assert(sizeof(long double) == 16);
#endif
        static_assert(sizeof(vince_csv::DataType) == 4);
        static_assert(sizeof(cell_span) == 16);

    }; //reader<>

    template <TrimPolicyConcept T, QuoteConcept Q, DelimiterConcept D, LineBreakConcept L, MaxFieldSizePolicyConcept M, EmptyRowsPolicyConcept E>
    template<typename T1, typename G, class ... Bases>
    void reader<T, Q, D, L, M, E>::promise_type_base<T1, G, Bases...>::
    unhandled_exception() {
        std::terminate();
    }

    static_assert(!std::is_copy_constructible<reader<>>::value);
    static_assert(!std::is_copy_assignable<reader<>>::value);

    static_assert(std::is_move_constructible<reader<>>::value);
    static_assert(std::is_move_assignable<reader<>>::value);

    std::ostream& operator<< (std::ostream& os, vince_csv::DataType const & d) {
        os << static_cast<int>(d);
        return os;
    }

} // namespace csv_co
