#include <gtest/gtest.h>

#include <string>
#include <vector>

import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.iterator;
import worse.core.container.map;

using namespace worse;
using namespace worse::core;
using namespace worse::core::container;

namespace
{
    template <typename M>
    std::vector<int> keys(M const& m)
    {
        std::vector<int> out;
        for (auto const& kv : m)
        {
            out.push_back(kv.first);
        }
        return out;
    }
} // namespace

TEST(MapTest, IndexInsertAndOrderedByKey)
{
    Map<int, int> m;
    m[3] = 30;
    m[1] = 10;
    m[2] = 20;
    m[1] = 11; // overwrite
    EXPECT_EQ(m.size(), 3u);
    EXPECT_EQ(keys(m), (std::vector<int>{1, 2, 3})); // sorted by key
    EXPECT_EQ(m[1], 11);
    EXPECT_EQ(m.at(2), 20);
}

TEST(MapTest, InsertInsertOrAssignTryEmplace)
{
    Map<int, int> m;
    auto r = m.insert({5, 50});
    EXPECT_TRUE(r.second);
    EXPECT_EQ(r.first->second, 50);
    EXPECT_FALSE(m.insert({5, 99}).second); // dup key, not assigned
    EXPECT_EQ(m.at(5), 50);

    auto a = m.insertOrAssign(5, 55);
    EXPECT_FALSE(a.second);
    EXPECT_EQ(m.at(5), 55);
    auto a2 = m.insertOrAssign(6, 60);
    EXPECT_TRUE(a2.second);
    EXPECT_EQ(m.at(6), 60);

    auto t = m.tryEmplace(7, 70);
    EXPECT_TRUE(t.second);
    EXPECT_EQ(m.at(7), 70);
    EXPECT_FALSE(m.tryEmplace(7, 999).second);
    EXPECT_EQ(m.at(7), 70);
}

TEST(MapTest, FindLookupBoundsErase)
{
    Map<int, int> m;
    for (int i = 1; i <= 5; ++i)
    {
        m[i * 10] = i;
    }
    EXPECT_TRUE(m.contains(30));
    EXPECT_EQ(m.find(20)->second, 2);
    EXPECT_EQ(m.find(99), m.end());
    EXPECT_EQ(m.lowerBound(25)->first, 30);
    EXPECT_EQ(m.upperBound(30)->first, 40);

    EXPECT_EQ(m.erase(30), 1u);
    EXPECT_EQ(m.erase(30), 0u);
    EXPECT_EQ(keys(m), (std::vector<int>{10, 20, 40, 50}));
    auto it = m.find(40);
    auto nx = m.erase(it);
    EXPECT_EQ(nx->first, 50);
}

TEST(MapTest, MutableSecondViaIterator)
{
    Map<int, int> m{{1, 1}, {2, 2}, {3, 3}};
    for (auto& kv : m)
    {
        kv.second *= 10;
    }
    EXPECT_EQ(m.at(1), 10);
    EXPECT_EQ(m.at(3), 30);
}

TEST(MapTest, CopyMoveSwap)
{
    Map<int, int> a{{1, 1}, {2, 2}};
    Map<int, int> b = a;
    b[3]            = 3;
    EXPECT_FALSE(a.contains(3));
    EXPECT_EQ(b.size(), 3u);

    Map<int, int> c = static_cast<Map<int, int>&&>(a);
    EXPECT_TRUE(a.empty());
    EXPECT_EQ(c.at(2), 2);

    Map<int, int> x{{1, 1}}, y{{9, 9}, {8, 8}};
    swap(x, y);
    EXPECT_EQ(keys(x), (std::vector<int>{8, 9}));
    EXPECT_EQ(keys(y), (std::vector<int>{1}));
}

TEST(MapTest, StringMappedOrdered)
{
    Map<int, std::string> m;
    m[2] = "two";
    m[1] = "one";
    m[3] = "three";
    std::string joined;
    for (auto const& kv : m)
    {
        joined += kv.second + ";";
    }
    EXPECT_EQ(joined, "one;two;three;"); // key order 1,2,3
}

namespace
{
    struct HKey
    {
        int v;
        friend bool operator<(HKey a, HKey b) noexcept { return a.v < b.v; }
        friend bool operator<(HKey a, int b) noexcept { return a.v < b; }
        friend bool operator<(int a, HKey b) noexcept { return a < b.v; }
    };
} // namespace

TEST(MapTest, TransparentHeterogeneousLookup)
{
    Map<HKey, int> m; // default Compare is the transparent Less<>
    m.insert({HKey{1}, 10});
    m.insert({HKey{2}, 20});
    // Look up by a bare int -> no temporary HKey is constructed (R45).
    EXPECT_TRUE(m.contains(2));
    EXPECT_EQ(m.count(2), 1u);
    EXPECT_FALSE(m.contains(3));
    auto it = m.find(2);
    ASSERT_NE(it, m.end());
    EXPECT_EQ(it->second, 20);
}
