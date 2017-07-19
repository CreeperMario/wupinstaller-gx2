#! /bin/bash
#
rev_new=$(git rev-parse --short=7 HEAD)

rev_old=$(cat ./src/gitrev.c 2>/dev/null | tr -d '\n' | awk -F"\"" '{print $2}' | awk -F"\"" '{print $1}')


if [ "$rev_new" != "$rev_old" ] || [ ! -f ./src/gitrev.c ]; then

	if [ -n "$rev_new" ]; then
		echo "Changed Rev $rev_old to $rev_new" >&2
	fi

    cat <<EOF > ./src/gitrev.c
#define GIT_REV "$rev_new"
const char *GetRev()
{
	return GIT_REV;
}
EOF

    rev_date=`date -u +%Y%m%d%H%M%S`

    cat <<EOF > ./meta/meta.xml
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<app version="1">
  <name>WUP Installer GX2</name>
  <coder>Dj_Skual</coder>
  <version>v1.1 CreeperMario-git-$rev_new</version>
  <release_date>$rev_date</release_date>
  <short_description>WiiU public title installer.</short_description> 
  <long_description>This application can install public titles such as games, game updates or DLC to your system memory (NAND) or the WiiU formatted USB.
This application is based on "WUP Installer y Mod" by Yardape and the GUI from "Loadiine GX2" sources.

This version work from HBL and HBL channel.

Thanks to:
  Loadiine GX2 team for GUI sources.
  Yardape for installer sources.

Links:
  https://sourceforge.net/projects/wup-installer-gx2/
  </long_description> 
</app>
EOF

fi

echo $rev_new
