#!/bin/sh
set -eu

if [ -x "${HOME}/config/non-packaged/bin/fatcat-cli" ]; then
	"${HOME}/config/non-packaged/bin/fatcat-cli" quit >/dev/null 2>&1 || true
fi
if command -v desklink >/dev/null 2>&1; then
	desklink --remove=FatCatDeskbar >/dev/null 2>&1 || true
fi
sleep 1

rm -f "${HOME}/config/non-packaged/add-ons/deskbar/FatCatDeskbar.so"
rm -f "${HOME}/config/non-packaged/add-ons/deskbar/FatCatDeskbar.so.new"
rm -f "${HOME}/config/non-packaged/bin/fatcat-cli"
rm -rf "${HOME}/config/non-packaged/apps/FatCat"
echo "Removed the application and Deskbar add-on. Settings were kept in ~/config/settings/FatCat."
