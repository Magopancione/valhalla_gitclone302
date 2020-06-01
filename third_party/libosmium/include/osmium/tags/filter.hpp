#ifndef OSMIUM_TAGS_FILTER_HPP
#define OSMIUM_TAGS_FILTER_HPP

/*

This file is part of Osmium (https://osmcode.org/libosmium).

Copyright 2013-2019 Jochen Topf <jochen@topf.org> and others (see README).

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

#include <osmium/memory/collection.hpp>
#include <osmium/osm/tag.hpp>

#include <boost/iterator/filter_iterator.hpp>

#include <cstddef>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

namespace osmium {

    namespace tags {

        template <typename TKey>
        struct match_key {
            bool operator()(const TKey& rule_key, const char* tag_key) const {
                return rule_key == tag_key;
            }
        }; // struct match_key

        template <>
        struct match_key<std::string> {
            bool operator()(const std::string& rule_key, const char* tag_key) const {
                return !std::strcmp(rule_key.c_str(), tag_key);
            }
        }; // struct match_key

        struct match_key_prefix {
            bool operator()(const std::string& rule_key, const char* tag_key) const {
                return rule_key.compare(0, std::string::npos, tag_key, 0, rule_key.size()) == 0;
            }
        }; // struct match_key_prefix

        template <typename TValue>
        struct match_value {
            bool operator()(const TValue& rule_value, const char* tag_value) const {
                return rule_value == tag_value;
            }
        }; // struct match_value

        template <>
        struct match_value<std::string> {
            bool operator()(const std::string& rule_value, const char* tag_value) const {
                return !std::strcmp(rule_value.c_str(), tag_value);
            }
        }; // struct match_value

        template <>
        struct match_value<void> {
            bool operator()(const bool /*rule_value*/, const char* /*tag_value*/) const noexcept {
                return true;
            }
        }; // struct match_value<void>

        /// @deprecated Use osmium::TagsFilter instead.
        template <typename TKey, typename TValue = void, typename TKeyComp = match_key<TKey>, typename TValueComp = match_value<TValue>>
        class Filter {

            using key_type   = TKey;
            using value_type = typename std::conditional<std::is_void<TValue>::value, bool, TValue>::type;

            struct Rule {
                key_type key;
                value_type value;
                bool ignore_value;
                bool result;

                explicit Rule(bool r, bool ignore, key_type k, value_type v) :
                    key(std::move(k)),
                    value(std::move(v)),
                    ignore_value(ignore),
                    result(r) {
                }

                explicit Rule(bool r, bool ignore, key_type k) :
                    key(std::move(k)),
                    value(),
                    ignore_value(ignore),
                    result(r) {
                }

            }; // struct Rule

            std::vector<Rule> m_rules;
            bool m_default_result;

        public:

            using filter_type   = Filter<TKey, TValue, TKeyComp, TValueComp>;
            using argument_type = const osmium::Tag&;
            using result_type   = bool;
            using iterator      = boost::filter_iterator<filter_type, osmium::TagList::const_iterator>;

            explicit Filter(bool default_result = false) :
                m_default_result(default_result) {
            }

            template <typename V = TValue, typename std::enable_if<!std::is_void<V>::value, int>::type = 0>
            Filter& add(bool result, const key_type& key, const value_type& value) {
                m_rules.emplace_back(result, false, key, value);
                return *this;
            }

            Filter& add(bool result, const key_type& key) {
                m_rules.emplace_back(result, true, key);
                return *this;
            }

            bool operator()(const osmium::Tag& tag) const {
                for (const Rule& rule : m_rules) {
                    if (TKeyComp()(rule.key, tag.key()) && (rule.ignore_value || TValueComp()(rule.value, tag.value()))) {
                        return rule.result;
                    }
                }
                return m_default_result;
            }

            /**
             * Return the number of rules in this filter.
             *
             * Complexity: Constant.
             */
            size_t count() const noexcept {
                return m_rules.size();
            }

            /**
             * Is this filter empty, ie are there no rules defined?
             *
             * Complexity: Constant.
             */
            bool empty() const noexcept {
                return m_rules.empty();
            }

        }; // class Filter

        /// @deprecated Use osmium::TagsFilter instead.
        using KeyValueFilter  = Filter<std::string, std::string>;

        /// @deprecated Use osmium::TagsFilter instead.
        using KeyFilter       = Filter<std::string>;

        /// @deprecated Use osmium::TagsFilter instead.
        using KeyPrefixFilter = Filter<std::string, void, match_key_prefix>;

    } // namespace tags

} // namespace osmium

#endif // OSMIUM_TAGS_FILTER_HPP