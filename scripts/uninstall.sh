#!/bin/sh
set -eu

rm -f "${HOME}/config/non-packaged/add-ons/deskbar/FatCatDeskbar.so"
rm -f "${HOME}/config/non-packaged/add-ons/deskbar/FatCatDeskbar.so.new"
rm -f "${HOME}/config/non-packaged/bin/fatcat-cli"
rm -rf "${HOME}/config/non-packaged/apps/FatCat"
echo "Removed the application and Deskbar add-on. Settings were kept in ~/config/settings/FatCat."
