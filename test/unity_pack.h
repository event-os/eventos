
#ifndef UNITY_PACK_H__
#define UNITY_PACK_H__

#include "unity.h"
#include "eventos.h"

#if (EOS_TEST_PLATFORM == 32)
#define TEST_ASSERT_EQUAL_POINTER       TEST_ASSERT_EQUAL_UINT32
#else
#define TEST_ASSERT_EQUAL_POINTER       TEST_ASSERT_EQUAL_UINT64
#endif

#endif
