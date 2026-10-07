#!/bin/sh
set -eu

app_dir="${HOME}/config/non-packaged/apps/FatCat"
deskbar_dir="${HOME}/config/non-packaged/add-ons/deskbar"
bin_dir="${HOME}/config/non-packaged/bin"
mkdir -p "$app_dir/assets" "$deskbar_dir" "$bin_dir"

./fatcat-cli quit >/dev/null 2>&1 || true
sleep 1
if command -v killall >/dev/null 2>&1; then
	killall fatcat >/dev/null 2>&1 || true
	sleep 1
	# A previous build may be blocked inside Deskbar IPC and unable to handle
	# either its quit message or SIGTERM. Ensure it cannot hold up replacement.
	killall -9 fatcat >/dev/null 2>&1 || true
fi
if command -v desklink >/dev/null 2>&1; then
	# Remove every stale copy left by older builds that repeatedly installed
	# the replicant. A fixed bound avoids depending on desklink's exit status.
	remove_attempt=0
	while [ "$remove_attempt" -lt 32 ]; do
		desklink --remove=fatcatDeskbar >/dev/null 2>&1 || true
		remove_attempt=$((remove_attempt + 1))
	done
	sleep 1
fi

cp fatcat "$app_dir/fatcat"
rm -f "$app_dir/fatcat.app"
cp fatcat-cli "$bin_dir/fatcat-cli"
cp assets/cat-orange.png assets/cat-gray.png assets/cat-calico.png assets/cat-tuxedo.png "$app_dir/assets/"
mimeset -f "$app_dir/fatcat"
mimeset -f "$bin_dir/fatcat-cli"
cp FatCatDeskbar.so "$deskbar_dir/FatCatDeskbar.so.new"
mimeset -f "$deskbar_dir/FatCatDeskbar.so.new"
mv "$deskbar_dir/FatCatDeskbar.so.new" "$deskbar_dir/FatCatDeskbar.so"
mimeset -f "$deskbar_dir/FatCatDeskbar.so"
"$app_dir/fatcat" --background >/dev/null 2>&1 &
echo "Installed Fat Cat in Deskbar. Its windows open on the active workspace."
