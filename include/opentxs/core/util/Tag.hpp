// Copyright (c) 2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_UTIL_TAG_HPP
#define OPENTXS_CORE_UTIL_TAG_HPP

#include "opentxs/Forward.hpp"

#include <string>
#include <map>
#include <vector>
#include <memory>

namespace opentxs
{

class Tag;

typedef std::shared_ptr<Tag> TagPtr;
typedef std::map<std::string, std::string> map_strings;
typedef std::vector<TagPtr> vector_tags;

class Tag
{
private:
    std::string name_;
    std::string text_;
    map_strings attributes_;
    vector_tags tags_;

public:
    const std::string& name() const { return name_; }
    const std::string& text() const { return text_; }
    const map_strings& attributes() const { return attributes_; }
    const vector_tags& tags() const { return tags_; }

    void set_name(const std::string& str_name) { name_ = str_name; }
    void set_text(const std::string& str_text) { text_ = str_text; }

    void add_attribute(
        const std::string& str_att_name,
        const std::string& str_att_value);

    void add_attribute(
        const std::string& str_att_name,
        const char* sz_att_value);

    void add_tag(TagPtr& tag_input);
    void add_tag(
        const std::string& str_tag_name,
        const std::string& str_tag_value);

    Tag(const std::string& str_name);

    Tag(const std::string& str_name, const std::string& str_text);

    Tag(const std::string& str_name, const char* sztext);

    void output(std::string& str_output) const;
    void outputXML(std::string& str_output) const;
};

}  // namespace opentxs

/*


 <HOME>

 <NICKNAME>Old Farmer's Ranch</NICKNAME>

 <ADDRESS
  number="6801"
  street1="Old Ranch Road"
  street2="unit 2567"
  postalcode="99781"
  city="Cracked Rock"
  state="UT"
  country="USA"
 />

 </HOME>


 */
#endif
