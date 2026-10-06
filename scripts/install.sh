#!/bin/sh
set -eu

app_dir="${HOME}/config/non-packaged/apps/FatCat"
deskbar_dir="${HOME}/config/non-packaged/add-ons/deskbar"
bin_dir="${HOME}/config/non-packaged/bin"
mkdir -p "$app_dir/assets" "$deskbar_dir" "$bin_dir"

./fatcat-cli quit >/dev/null 2>&1 || true
sleep 1
if command -v killall >/dev/null 2>&1; then
	killall fatcat.app >/dev/null 2>&1 || true
fi
if command -v desklink >/dev/null 2>&1; then
	desklink --remove=FatCatDeskbar >/dev/null 2>&1 || true
	sleep 1
fi

cp fatcat.app "$app_dir/fatcat.app"
cp fatcat-cli "$bin_dir/fatcat-cli"
cp assets/cat-orange.png assets/cat-gray.png assets/cat-calico.png assets/cat-tuxedo.png "$app_dir/assets/"
mimeset -f "$app_dir/fatcat.app"
mimeset -f "$bin_dir/fatcat-cli"
cp FatCatDeskbar.so "$deskbar_dir/FatCatDeskbar.so.new"
mimeset -f "$deskbar_dir/FatCatDeskbar.so.new"
mv "$deskbar_dir/FatCatDeskbar.so.new" "$deskbar_dir/FatCatDeskbar.so"
mimeset -f "$deskbar_dir/FatCatDeskbar.so"
"$app_dir/fatcat.app" --background >/dev/null 2>&1 &
echo "Installed Fat Cat in Deskbar. Its windows open on the active workspace."
