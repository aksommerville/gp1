#!/bin/bash

# log in color
log_raw()    { echo "$1" ; }
log_detail() { echo -e "\x1b[31m$1\x1b[0m" ; }
log_fail()   { echo -e "\x1b[41mFAIL\x1b[0m $1" ; }
log_pass()   { echo -e "\x1b[42mPASS\x1b[0m $1" ; }
log_skip()   { echo -e "\x1b[100mSKIP\x1b[0m $1" ; }

# no color
#log_raw()    { echo "$1" ; }
#log_detail() { echo "$1" ; }
#log_fail()   { echo "FAIL $1" ; }
#log_pass()   { echo "PASS $1" ; }
#log_skip()   { echo "SKIP $1" ; }
  
# silent
#log_raw()    { true ; }
#log_detail() { true ; }
#log_fail()   { true ; }
#log_pass()   { true ; }
#log_skip()   { true ; }

FAILC=0
PASSC=0
SKIPC=0

run_test() {
  while IFS= read INPUT ; do
    read INTRODUCER KEYWORD TEXT <<<"$INPUT"
    if [ "$INTRODUCER" = GP1_TEST ] ; then
      case "$KEYWORD" in
        DETAIL) log_detail "$TEXT" ;;
        FAIL) FAILC=$((FAILC+1)) ; log_fail "$TEXT" ;;
        PASS) PASSC=$((PASSC+1)) ; log_pass "$TEXT" ;;
        SKIP) SKIPC=$((SKIPC+1)) ; log_skip "$TEXT" ;;
        *) log_raw "$INPUT" ;;
      esac
    else
      log_raw "$INPUT"
    fi
  done < <( $1 $GP1_TEST_FILTER 2>&1 || echo "GP1_TEST FAIL $1" )
}

for EXE in $* ; do
  run_test "$EXE"
done

FLAG=
STATUS=0

if [ "$FAILC" -gt 0 ] ; then
  FLAG="\x1b[41m    \x1b[0m"
  STATUS=1
elif [ "$PASSC" -gt 0 ] ; then
  FLAG="\x1b[42m    \x1b[0m"
else
  FLAG="\x1b[100m    \x1b[0m"
fi

echo -e "$FLAG $FAILC fail, $PASSC pass, $SKIPC skip"

exit $STATUS
