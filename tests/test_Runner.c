/* AUTOGENERATED FILE. DO NOT EDIT. */

//=======Test Runner Used To Run Each Test Below=====
#define RUN_TEST(TestFunc, TestLineNum) \
{ \
  Unity.CurrentTestName = #TestFunc; \
  Unity.CurrentTestLineNumber = TestLineNum; \
  Unity.NumberOfTests++; \
  if (TEST_PROTECT()) \
  { \
      setUp(); \
      TestFunc(); \
  } \
  if (TEST_PROTECT() && !TEST_IS_IGNORED) \
  { \
    tearDown(); \
  } \
  UnityConcludeTest(); \
}

//=======Automagically Detected Files To Include=====
#include "unity.h"
#include <setjmp.h>
#include <stdio.h>
#include "vmod_geo.h"
#include <string.h>
#include <maxminddb.h>
#include <float.h>

//=======External Functions This Runner Calls=====
extern void setUp(void);
extern void tearDown(void);
extern void test_OpenMMDB(void);
extern void test_BadIP(void);
extern void test_CaliforniaIP(void);
extern void test_ParisFranceIP(void);
extern void test_LookupCity(void);
extern void test_LookupState(void);
extern void test_LookupCountry(void);
extern void test_GetWeatherCode(void);
extern void test_GetCookie(void);
extern void test_GetEmptyCookie();
extern void test_GetEmptyCookieA();
extern void test_GetEmptyCookieAb();
extern void test_GetEmptyCookieB();
extern void test_GetEmptyCookieC();
extern void test_GetEmptyCookieD();
extern void test_GetEmptyCookieE();
extern void test_GetEmptyCookieF();


//=======Test Reset Option=====
void resetTest()
{
  tearDown();
  setUp();
}


//=======MAIN=====
int main(void)
{
  UnityBegin();
  Unity.TestFile = "tests.c";
  RUN_TEST(test_OpenMMDB, 22);
  RUN_TEST(test_BadIP, 28);
  RUN_TEST(test_CaliforniaIP, 36);
  RUN_TEST(test_ParisFranceIP, 44);
  RUN_TEST(test_LookupCity, 52);
  RUN_TEST(test_LookupState, 61);
  RUN_TEST(test_LookupCountry, 70);
  RUN_TEST(test_GetWeatherCode, 79);
  RUN_TEST(test_GetCookie, 104);
  RUN_TEST(test_GetEmptyCookie, 114);
  RUN_TEST(test_GetEmptyCookieA, 123);
  RUN_TEST(test_GetEmptyCookieAb, 131);
  RUN_TEST(test_GetEmptyCookieB, 141);
  RUN_TEST(test_GetEmptyCookieC, 153);
  RUN_TEST(test_GetEmptyCookieD, 165);
  RUN_TEST(test_GetEmptyCookieE, 175);
  RUN_TEST(test_GetEmptyCookieF, 185);

  return (UnityEnd());
}
