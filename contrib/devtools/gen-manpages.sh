#!/bin/sh

TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
SRCDIR=${SRCDIR:-$TOPDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

EVRMORED=${EVRMORED:-$SRCDIR/evrmored}
EVRMORECLI=${EVRMORECLI:-$SRCDIR/evrmore-cli}
EVRMORETX=${EVRMORETX:-$SRCDIR/evrmore-tx}
EVRMOREQT=${EVRMOREQT:-$SRCDIR/qt/evrmore-qt}

[ ! -x $EVRMORED ] && echo "$EVRMORED not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
EVRVER=($($EVRMORECLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }'))

# Create a footer file with copyright content.
# This gets autodetected fine for evrmored if --version-string is not set,
# but has different outcomes for evrmore-qt and evrmore-cli.
echo "[COPYRIGHT]" > footer.h2m
$EVRMORED --version | sed -n '1!p' >> footer.h2m

for cmd in $EVRMORED $EVRMORECLI $EVRMORETX $EVRMOREQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${EVRVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${EVRVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m
