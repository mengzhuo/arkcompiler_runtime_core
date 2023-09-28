/**
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "util/enum_tag.h"
#include "util/int_tag.h"
#include "util/tagged_index.h"
#include "util/tests/verifier_test.h"

#include <gtest/gtest.h>

#include <unordered_set>

namespace panda::verifier::test {

TEST_F(VerifierTest, Tagged_index)
{
    enum class Tag { TAG0, TAG1, TAG2 };
    using TagType1 = TagForEnum<Tag, Tag::TAG0, Tag::TAG1, Tag::TAG2>;
    // NOLINTNEXTLINE(readability-magic-numbers)
    using TagType2 = TagForInt<int, 5, 10>;
    TaggedIndex<TagType1, TagType2, size_t> tagind1 {};
    EXPECT_FALSE(tagind1.IsValid());
    tagind1.SetTag<0>(Tag::TAG1);
    tagind1.SetTag<1>(7);
    tagind1.SetInt(8);
    ASSERT_TRUE(tagind1.IsValid());
    EXPECT_EQ(tagind1.GetTag<0>(), Tag::TAG1);
    EXPECT_EQ(tagind1.GetTag<1>(), 7);
    EXPECT_EQ(tagind1, 8);
    tagind1.Invalidate();
    EXPECT_FALSE(tagind1.IsValid());

    TaggedIndex<TagType2, TagType1> tagind2 {};
    EXPECT_FALSE(tagind2.IsValid());
    tagind2.SetTag<1>(Tag::TAG1);
    tagind2.SetTag<0>(7);
    tagind2.SetInt(8);
    ASSERT_TRUE(tagind2.IsValid());
    EXPECT_EQ(tagind2.GetTag<1>(), Tag::TAG1);
    EXPECT_EQ(tagind2.GetTag<0>(), 7);
    EXPECT_EQ(tagind2, 8);
    tagind2.Invalidate();
    EXPECT_FALSE(tagind2.IsValid());
}

TEST_F(VerifierTest, Tagged_index_in_container)
{
    enum class Tag { TAG0, TAG1, TAG2 };
    using TagType1 = TagForEnum<Tag, Tag::TAG0, Tag::TAG1, Tag::TAG2>;
    // NOLINTNEXTLINE(readability-magic-numbers)
    using TagType2 = TagForInt<int, 5, 10>;
    using TI = TaggedIndex<TagType1, TagType2, size_t>;
    TI tagind1 {};
    std::unordered_set<TI> i_set {};
    tagind1.SetTag<0>(Tag::TAG1);
    tagind1.SetTag<1>(7);
    tagind1.SetInt(8);
    i_set.insert(tagind1);
    tagind1.SetTag<0>(Tag::TAG2);
    // NOLINTNEXTLINE(readability-magic-numbers)
    tagind1.SetTag<1>(9);
    tagind1.SetInt(3);
    i_set.insert(tagind1);
    tagind1.SetTag<0>(Tag::TAG1);
    tagind1.SetTag<1>(7);
    tagind1.SetInt(8);
    EXPECT_EQ(i_set.count(tagind1), 1);
    tagind1.SetTag<0>(Tag::TAG2);
    // NOLINTNEXTLINE(readability-magic-numbers)
    tagind1.SetTag<1>(9);
    tagind1.SetInt(3);
    EXPECT_EQ(i_set.count(tagind1), 1);
    tagind1.SetInt(4);
    EXPECT_EQ(i_set.count(tagind1), 0);
}

}  // namespace panda::verifier::test
