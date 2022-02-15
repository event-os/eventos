#ifndef __M_ASSERT_H__
#define __M_ASSERT_H__

void m_assert(const char * module, const char * name, int id);
void n_assert(const char * module, const char * name, int id);

#ifdef M_NASSERT

#define M_MODULE_NAME(name_)
#define M_ASSERT(test_)                         ((void)0)
#define M_ASSERT_MEM(pBuff, pBuffRef, size)     ((void)0)
#define M_ASSERT_ID(id_, test_)                 ((void)0)
#define M_ALLEGE(test_)                         ((void)(test_))
#define M_ALLEGE_ID(id_, test_)                 ((void)(test_))
#define M_ERROR_ID(id_)                         ((void)0)

#else

// 定义用户指定的模块名称。
#define M_MODULE_NAME(name_)                                                   \
    static char const Module_Name[] = name_;

// 通用断言
#define S_ASSERT(test_) ((test_)                                               \
    ? (void)0 : s_assert(&Module_Name[0], 0, (int)__LINE__))

// 通用断言
#define M_ASSERT(test_) ((test_)                                               \
    ? (void)0 : m_assert(&Module_Name[0], 0, (int)__LINE__))

// 用于检查某块内存的断言
#define M_ASSERT_MEM(pBuff, pBuffRef, size) do {                               \
    for (uint32_t i = 0; i < (size); i ++)                                     \
        if (*(((uint8_t *)(pBuff)) + i) != *(((uint8_t *)(pBuffRef)) + i))     \
            m_assert(&Module_Name[0], 0, (int)__LINE__);                       \
} while (0)

// 携带用于指定ID的通用断言。
#define M_ASSERT_ID(id_, test_) ((test_)                                       \
    ? (void)0 : m_assert(&Module_Name[0], 0, (int)(id_)))

// 携带用于某设备名称的通用断言。
#define M_ASSERT_NAME(id_, name_) ((test_)                                     \
    ? (void)0 : m_assert(&Module_Name[0], name_, (int)(__LINE__)))

// 通用断言，无法被关闭
#define M_ALLEGE(test_)    M_ASSERT(test_)

// 携带用于指定ID的通用断言，无法被关闭。
#define M_ALLEGE_ID(id_, test_) M_ASSERT_ID((id_), (test_))

// 携带ID的断言，用于进入一个错误的路径。
#define M_ERROR_ID(id_)                                                        \
    m_assert(&Module_Name[0], (int)(id_))

#endif

#endif