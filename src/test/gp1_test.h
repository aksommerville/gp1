/* gp1_test.h
 */
 
#ifndef GP1_TEST_H
#define GP1_TEST_H

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

/* Declare tests.
 ****************************************************************/

/* Invoke at global scope, and follow immediately with the function body.
 * If the file is located under src/test/int/, make will notice and rebuild the TOC.
 */
#define GP1_ITEST(name,...) int name()
#define XXX_GP1_ITEST(name,...) int name()

#define GP1_UTEST_PRIVATE(enable,fn,...) \
  if (!gp1_test_filter(argc,argv,#fn,#__VA_ARGS__,__FILE__,enable)) { \
    fprintf(stderr,"GP1_TEST SKIP %s [%s:%d]\n",#fn,__FILE__,__LINE__); \
  } else if (fn()<0) { \
    fprintf(stderr,"GP1_TEST FAIL %s [%s:%d]\n",#fn,__FILE__,__LINE__); \
  } else { \
    fprintf(stderr,"GP1_TEST PASS %s [%s:%d]\n",#fn,__FILE__,__LINE__); \
  }

/* Invoke from your unit test's main().
 */
#define GP1_UTEST(fn,...) GP1_UTEST_PRIVATE(1,fn,##__VA_ARGS__)
#define XXX_GP1_UTEST(fn,...) GP1_UTEST_PRIVATE(0,fn,##__VA_ARGS__)

int gp1_test_filter(
  int argc,char **argv, // exactly as provided to main()
  const char *name, // name of test, ie function name
  const char *tags, // comma-delimited tags belonging to this test
  const char *file, // path or basename of file containing test
  int enable // test's default disposition (args may override)
);

/* Structured logging.
 *********************************************************************/
 
// Redefine if you need to.
#define GP1_FAIL_RETURN_VALUE -1

#define GP1_FAIL_MORE(k,fmt,...) fprintf(stderr,"GP1_TEST DETAIL | %20s: "fmt"\n",k,##__VA_ARGS__);

#define GP1_FAIL_BEGIN(fmt,...) { \
  fprintf(stderr,"GP1_TEST DETAIL +--------------------------------------------------------\n"); \
  fprintf(stderr,"GP1_TEST DETAIL | Assertion failed\n"); \
  fprintf(stderr,"GP1_TEST DETAIL +--------------------------------------------------------\n"); \
  if (fmt&&fmt[0]) GP1_FAIL_MORE("Message",fmt,##__VA_ARGS__) \
  GP1_FAIL_MORE("Location","%s:%d",__FILE__,__LINE__) \
}

#define GP1_FAIL_END { \
  fprintf(stderr,"GP1_TEST DETAIL +--------------------------------------------------------\n"); \
  return GP1_FAIL_RETURN_VALUE; \
}

/* Assertions.
 ************************************************************************/

#define GP1_FAIL(fmt,...) { \
  GP1_FAIL_BEGIN(fmt,##__VA_ARGS__) \
  GP1_FAIL_END \
}

#define GP1_ASSERT(condition,...) if (!(condition)) { \
  GP1_FAIL_BEGIN(""__VA_ARGS__) \
  GP1_FAIL_MORE("Expected","true") \
  GP1_FAIL_MORE("As written","%s",#condition) \
  GP1_FAIL_END \
}

#define GP1_ASSERT_NOT(condition,...) if (condition) { \
  GP1_FAIL_BEGIN(""__VA_ARGS__) \
  GP1_FAIL_MORE("Expected","false") \
  GP1_FAIL_MORE("As written","%s",#condition) \
  GP1_FAIL_END \
}

#define GP1_ASSERT_CALL(call,...) { \
  int _result=(call); \
  if (_result<0) { \
    GP1_FAIL_BEGIN(""__VA_ARGS__) \
    GP1_FAIL_MORE("Expected","success") \
    GP1_FAIL_MORE("As written","%s",#call) \
    GP1_FAIL_MORE("Result","%d",_result) \
    GP1_FAIL_END \
  } \
}

#define GP1_ASSERT_FAILURE(call,...) { \
  int _result=(call); \
  if (_result>=0) { \
    GP1_FAIL_BEGIN(""__VA_ARGS__) \
    GP1_FAIL_MORE("Expected","failure") \
    GP1_FAIL_MORE("As written","%s",#call) \
    GP1_FAIL_MORE("Result","%d",_result) \
    GP1_FAIL_END \
  } \
}

#define GP1_ASSERT_INTS_OP(a,op,b,...) { \
  int _a=(int)(a),_b=(int)(b); \
  if (!(_a op _b)) { \
    GP1_FAIL_BEGIN(""__VA_ARGS__) \
    GP1_FAIL_MORE("As written","%s %s %s",#a,#op,#b) \
    GP1_FAIL_MORE("Values","%d %s %d",_a,#op,_b) \
    GP1_FAIL_END \
  } \
}

#define GP1_ASSERT_INTS(a,b,...) GP1_ASSERT_INTS_OP(a,==,b,##__VA_ARGS__)

#define GP1_ASSERT_STRINGS(a,ac,b,bc,...) { \
  const char *_a=(char*)(a),*_b=(char*)(b); \
  int _ac=(int)(ac); if (!_a) _ac=0; else if (_ac<0) { _ac=0; while (_a[_ac]) _ac++; } \
  int _bc=(int)(bc); if (!_b) _bc=0; else if (_bc<0) { _bc=0; while (_b[_bc]) _bc++; } \
  if ((_ac!=_bc)||memcmp(_a,_b,_ac)) { \
    GP1_FAIL_BEGIN(""__VA_ARGS__) \
    GP1_FAIL_MORE("(A) As written","%s : %s",#a,#ac) \
    GP1_FAIL_MORE("(B) As written","%s : %s",#b,#bc) \
    GP1_FAIL_MORE("(A) Value","%.*s",_ac,_a) \
    GP1_FAIL_MORE("(B) Value","%.*s",_bc,_b) \
    GP1_FAIL_END \
  } \
}

#endif
