#!/bin/sh

if [ "$#" -lt 1 ] ; then
  echo "Usage: $0 [INPUTS...] OUTPUT"
  exit 1
fi

TMPTOC=mid/tmptoc
echo -n "" >$TMPTOC

while [ "$#" -gt 1 ] ; do
  SRCPATH="$1"
  shift 1
  SRCBASE="$(basename $SRCPATH)"
  nl -ba -s' ' "$SRCPATH" \
    | sed -En 's/^ *([0-9]+) *(XXX_)?GP1_ITEST *\( *([0-9a-zA-Z_]+) *(, *([^)]*))?\).*$/'"$SRCBASE"' \1 _\2 \3 \5/p' \
    >>$TMPTOC
done

DSTPATH="$1"

cat - >"$DSTPATH" <<EOF
/* gp1_itest_toc.h
 * Generated at $(date).
 */
 
#ifndef GP1_ITEST_TOC_H
#define GP1_ITEST_TOC_H

struct gp1_itest {
  int (*fn)();
  const char *name;
  const char *tags;
  const char *file;
  int line;
  int enable;
};

EOF

while read F L I N T ; do
  echo "int $N();" >>"$DSTPATH"
done <$TMPTOC

cat - >>"$DSTPATH" <<EOF

static const struct gp1_itest gp1_itestv[]={
EOF

while read F L I N T ; do
  if [ "$I" = "_" ] ; then
    ENABLE=1
  else
    ENABLE=0
  fi
  echo "  {$N,\"$N\",\"$T\",\"$F\",$L,$ENABLE}," >>"$DSTPATH"
done <$TMPTOC

cat - >>"$DSTPATH" <<EOF
};

#endif
EOF

rm $TMPTOC
