/*****************************************************************************
 * Licensed to Qualys, Inc. (QUALYS) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * QUALYS licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ****************************************************************************/

/**
 * @file
 * @brief IronBee++ Internals --- Memory Manage Tests
 *
 * @author Christopher Alfeld <calfeld@qualys.com>
 **/

#include <ironbeepp/memory_manager.hpp>
#include <ironbeepp/memory_pool_lite.hpp>

#include <algorithm>

#include <boost/bind.hpp>

#include "gtest/gtest.h"

using namespace std;
using namespace IronBee;

// test_memory_manager.c tests the underlying C code.  This primarily tests
// the interface.

TEST(TestMemoryManager, Singular)
{
    MemoryManager mm;

    EXPECT_FALSE(mm);
}

TEST(TestMemoryManager, Allocations)
{
    ScopedMemoryPoolLite smpl;
    MemoryManager mm = MemoryPoolLite(smpl);
    void* p;
    char* c;

    ASSERT_TRUE(mm);

    p = NULL;
    p = mm.alloc(10);
    EXPECT_TRUE(p);

    p = NULL;
    p = mm.allocate<int>();
    EXPECT_TRUE(p);

    c = NULL;
    c = reinterpret_cast<char*>(mm.calloc(10));
    EXPECT_EQ(10L, count(c, c+10, '\0'));

    c = NULL;
    c = reinterpret_cast<char*>(mm.calloc(5, 7));
    EXPECT_EQ(35L, count(c, c+35, '\0'));

    static const string c_example = "Hello World";

    c = NULL;
    c = mm.strdup("Hello World");
    EXPECT_EQ(c_example, c);

    c = NULL;
    c = reinterpret_cast<char*>(
        mm.memdup(c_example.data(), c_example.size())
    );
    EXPECT_EQ(c_example, string(c, c_example.size()));

    c = NULL;
    c = mm.memdup_to_str(c_example.data(), c_example.size());
    EXPECT_EQ(c_example, c);
}

namespace {

void test_callback(bool* b)
{
    *b = true;
}

}

TEST(TestMemoryManager, Callback)
{
    bool called = false;

    MemoryPoolLite mpl = MemoryPoolLite::create();
    MemoryManager mm = mpl;

    mm.register_cleanup(
        boost::bind(test_callback, &called)
    );

    mpl.destroy();

    ASSERT_TRUE(called);
}
