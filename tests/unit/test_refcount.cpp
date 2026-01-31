//    SAPF - Sound As Pure Form
//    Copyright (C) 2019 James McCartney
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

#include <gtest/gtest.h>
#include "VM.hpp"  // Needed for RCObj::retain/release inline definitions

// Test fixture for reference counting tests
class RefCountTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Nothing to set up
    }

    void TearDown() override {
        // Nothing to tear down
    }
};

// A simple concrete class for testing reference counting
class TestObject : public Object {
public:
    static int constructCount;
    static int destructCount;

    TestObject() { constructCount++; }
    ~TestObject() override { destructCount++; }

    const char* TypeName() const override { return "TestObject"; }

    static void resetCounters() {
        constructCount = 0;
        destructCount = 0;
    }
};

int TestObject::constructCount = 0;
int TestObject::destructCount = 0;

//==============================================================================
// Basic retain/release tests
//==============================================================================

TEST_F(RefCountTest, NewObjectHasRefcountZero) {
    TestObject::resetCounters();
    TestObject* obj = new TestObject();
    EXPECT_EQ(obj->getRefcount(), 0);
    delete obj;  // Manual delete since refcount is 0
}

TEST_F(RefCountTest, RetainIncrementsRefcount) {
    TestObject::resetCounters();
    TestObject* obj = new TestObject();
    EXPECT_EQ(obj->getRefcount(), 0);

    obj->retain();
    EXPECT_EQ(obj->getRefcount(), 1);

    obj->retain();
    EXPECT_EQ(obj->getRefcount(), 2);

    obj->release();
    obj->release();  // Should delete
}

TEST_F(RefCountTest, ReleaseDecrementsRefcount) {
    TestObject::resetCounters();
    TestObject* obj = new TestObject();
    obj->retain();
    obj->retain();
    EXPECT_EQ(obj->getRefcount(), 2);

    obj->release();
    EXPECT_EQ(obj->getRefcount(), 1);

    obj->release();  // Should delete
    EXPECT_EQ(TestObject::destructCount, 1);
}

TEST_F(RefCountTest, ReleaseDeletesAtZero) {
    TestObject::resetCounters();
    TestObject* obj = new TestObject();
    obj->retain();
    EXPECT_EQ(TestObject::destructCount, 0);

    obj->release();
    EXPECT_EQ(TestObject::destructCount, 1);
}

//==============================================================================
// P<T> smart pointer tests
//==============================================================================

TEST_F(RefCountTest, SmartPointerRetainsOnConstruction) {
    TestObject::resetCounters();
    {
        P<TestObject> ptr(new TestObject());
        EXPECT_EQ(ptr->getRefcount(), 1);
    }
    EXPECT_EQ(TestObject::destructCount, 1);
}

TEST_F(RefCountTest, SmartPointerReleasesOnDestruction) {
    TestObject::resetCounters();
    {
        P<TestObject> ptr(new TestObject());
        EXPECT_EQ(TestObject::destructCount, 0);
    }
    EXPECT_EQ(TestObject::destructCount, 1);
}

TEST_F(RefCountTest, SmartPointerCopyIncrementsRefcount) {
    TestObject::resetCounters();
    P<TestObject> ptr1(new TestObject());
    EXPECT_EQ(ptr1->getRefcount(), 1);

    P<TestObject> ptr2 = ptr1;
    EXPECT_EQ(ptr1->getRefcount(), 2);
    EXPECT_EQ(ptr2->getRefcount(), 2);
    EXPECT_EQ(ptr1(), ptr2());  // Same underlying pointer
}

TEST_F(RefCountTest, SmartPointerAssignmentUpdatesRefcount) {
    TestObject::resetCounters();
    P<TestObject> ptr1(new TestObject());
    P<TestObject> ptr2(new TestObject());

    EXPECT_EQ(ptr1->getRefcount(), 1);
    EXPECT_EQ(ptr2->getRefcount(), 1);

    ptr1 = ptr2;

    EXPECT_EQ(ptr2->getRefcount(), 2);
    EXPECT_EQ(TestObject::destructCount, 1);  // First object deleted
}

TEST_F(RefCountTest, SmartPointerMoveDoesNotChangeRefcount) {
    TestObject::resetCounters();
    P<TestObject> ptr1(new TestObject());
    EXPECT_EQ(ptr1->getRefcount(), 1);

    P<TestObject> ptr2 = std::move(ptr1);
    EXPECT_EQ(ptr2->getRefcount(), 1);
    EXPECT_EQ(ptr1(), nullptr);  // ptr1 is now null
}

TEST_F(RefCountTest, SmartPointerNullConstruction) {
    P<TestObject> ptr;
    EXPECT_EQ(ptr(), nullptr);
    EXPECT_FALSE(ptr);
}

TEST_F(RefCountTest, SmartPointerBoolConversion) {
    P<TestObject> nullPtr;
    P<TestObject> validPtr(new TestObject());

    EXPECT_FALSE(nullPtr);
    EXPECT_TRUE(validPtr);
}

TEST_F(RefCountTest, SmartPointerSelfAssignment) {
    TestObject::resetCounters();
    P<TestObject> ptr(new TestObject());
    TestObject* rawPtr = ptr();

    ptr = ptr;  // Self-assignment

    EXPECT_EQ(ptr(), rawPtr);
    EXPECT_EQ(ptr->getRefcount(), 1);
    EXPECT_EQ(TestObject::destructCount, 0);
}

TEST_F(RefCountTest, SmartPointerSwap) {
    TestObject::resetCounters();
    P<TestObject> ptr1(new TestObject());
    P<TestObject> ptr2(new TestObject());

    TestObject* raw1 = ptr1();
    TestObject* raw2 = ptr2();

    ptr1.swap(ptr2);

    EXPECT_EQ(ptr1(), raw2);
    EXPECT_EQ(ptr2(), raw1);
    EXPECT_EQ(ptr1->getRefcount(), 1);
    EXPECT_EQ(ptr2->getRefcount(), 1);
}

//==============================================================================
// Multiple references tests
//==============================================================================

TEST_F(RefCountTest, MultipleSmartPointersShareOwnership) {
    TestObject::resetCounters();
    P<TestObject> ptr1(new TestObject());
    P<TestObject> ptr2 = ptr1;
    P<TestObject> ptr3 = ptr1;

    EXPECT_EQ(ptr1->getRefcount(), 3);

    ptr1 = nullptr;
    EXPECT_EQ(ptr2->getRefcount(), 2);
    EXPECT_EQ(TestObject::destructCount, 0);

    ptr2 = nullptr;
    EXPECT_EQ(ptr3->getRefcount(), 1);
    EXPECT_EQ(TestObject::destructCount, 0);

    ptr3 = nullptr;
    EXPECT_EQ(TestObject::destructCount, 1);
}

TEST_F(RefCountTest, SmartPointerSetMethod) {
    TestObject::resetCounters();
    P<TestObject> ptr;

    TestObject* obj = new TestObject();
    ptr.set(obj);

    EXPECT_EQ(ptr(), obj);
    EXPECT_EQ(obj->getRefcount(), 1);
}

//==============================================================================
// Thread safety (basic tests - full thread safety tested elsewhere)
//==============================================================================

TEST_F(RefCountTest, AtomicRefcountType) {
    // Verify refcount is atomic
    TestObject* obj = new TestObject();
    EXPECT_TRUE(obj->refcount.is_lock_free());
    delete obj;
}
