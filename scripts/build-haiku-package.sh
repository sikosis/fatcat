#!/bin/sh

set -eu

scriptDirectory=$(CDPATH= cd "$(dirname "$0")" && pwd)
projectDirectory=$(CDPATH= cd "$scriptDirectory/.." && pwd)
prepareOnly=false

if [ "${1:-}" = "--prepare-only" ]; then
	prepareOnly=true
	shift
fi

version=$(sed -n 's/^constexpr const char\* kAppVersion = "\([^"]*\)";/\1/p' \
	"$projectDirectory/src/Messages.h")

if [ -z "$version" ]; then
	echo "Could not read the Fat Cat version from src/Messages.h." >&2
	exit 1
fi

recipeTemplate="$projectDirectory/packaging/haikuports/fatcat-$version.recipe.in"
outputDirectory="$projectDirectory/dist"
archive="$outputDirectory/fatcat-$version.tar.gz"
recipe="$outputDirectory/fatcat-$version.recipe"

if [ ! -f "$recipeTemplate" ]; then
	echo "Missing recipe template: $recipeTemplate" >&2
	exit 1
fi

homepage=${FATCAT_HOMEPAGE:-}
if [ -z "$homepage" ] && command -v git >/dev/null 2>&1; then
	homepage=$(git -C "$projectDirectory" config --get remote.origin.url || true)
fi

case "$homepage" in
	git@github.com:*)
		homepage="https://github.com/${homepage#git@github.com:}"
		;;
	ssh://git@github.com/*)
		homepage="https://github.com/${homepage#ssh://git@github.com/}"
		;;
esac
homepage=${homepage%.git}

case "$homepage" in
	http://*|https://*)
		;;
	*)
		echo "Set FATCAT_HOMEPAGE to the public project URL." >&2
		exit 1
		;;
esac

temporaryDirectory=$(mktemp -d "${TMPDIR:-/tmp}/fatcat-haikuports.XXXXXX")
trap 'rm -rf "$temporaryDirectory"' EXIT HUP INT TERM
sourceDirectory="$temporaryDirectory/fatcat-$version"
mkdir -p "$sourceDirectory" "$outputDirectory"

(
	cd "$projectDirectory"
	tar \
		--exclude='./.git' \
		--exclude='./build' \
		--exclude='./dist' \
		--exclude='./fatcat' \
		--exclude='./fatcat-cli' \
		--exclude='./FatCatDeskbar.so' \
		--exclude='./tests/session_test' \
		--exclude='*.o' \
		--exclude='*.rsrc' \
		--exclude='.DS_Store' \
		-cf - .
) | (
	cd "$sourceDirectory"
	tar -xf -
)

# Keep the archive checksum stable when the source files have not changed.
touch -r "$recipeTemplate" "$sourceDirectory"

tarFile="$temporaryDirectory/fatcat-$version.tar"
tar -cf "$tarFile" -C "$temporaryDirectory" "fatcat-$version"
gzip -n -c "$tarFile" > "$archive"

if command -v sha256sum >/dev/null 2>&1; then
	checksum=$(sha256sum "$archive" | awk '{print $1}')
elif command -v sha256 >/dev/null 2>&1; then
	checksum=$(sha256 "$archive" | awk '{print $NF}')
elif command -v shasum >/dev/null 2>&1; then
	checksum=$(shasum -a 256 "$archive" | awk '{print $1}')
else
	echo "A SHA-256 utility (sha256sum, sha256, or shasum) is required." >&2
	exit 1
fi

escapeReplacement()
{
	printf '%s' "$1" | sed 's/[&|\\]/\\&/g'
}

escapedHomepage=$(escapeReplacement "$homepage")
escapedArchive=$(escapeReplacement "$archive")
escapedChecksum=$(escapeReplacement "$checksum")
escapedSourceDirectory=$(escapeReplacement "fatcat-$version")

sed \
	-e "s|@HOMEPAGE@|$escapedHomepage|g" \
	-e "s|@SOURCE_URI@|file://$escapedArchive|g" \
	-e "s|@CHECKSUM_SHA256@|$escapedChecksum|g" \
	-e "s|@SOURCE_DIR@|$escapedSourceDirectory|g" \
	"$recipeTemplate" > "$recipe"

echo "Created $archive"
echo "Created $recipe"

if [ "$prepareOnly" = true ]; then
	exit 0
fi

haikuportsTree=${HAIKUPORTS_TREE:-}
if [ -z "$haikuportsTree" ]; then
	haikuportsConfiguration="${HOME}/config/settings/haikuports.conf"
	if [ -f "$haikuportsConfiguration" ]; then
		haikuportsTree=$(awk -F= '
			/^[[:space:]]*TREE_PATH=/ {
				value = $2
				gsub(/^[[:space:]\"]+|[[:space:]\"]+$/, "", value)
				print value
				exit
			}' "$haikuportsConfiguration")
	fi
fi
haikuportsTree=${haikuportsTree:-/boot/home/haikuports}

if [ ! -d "$haikuportsTree" ]; then
	echo "HaikuPorts tree not found: $haikuportsTree" >&2
	echo "Set HAIKUPORTS_TREE to its location." >&2
	exit 1
fi

haikuporter=${HAIKUPORTER:-haikuporter}
if ! command -v "$haikuporter" >/dev/null 2>&1; then
	echo "HaikuPorter was not found: $haikuporter" >&2
	echo "Set HAIKUPORTER to the executable path." >&2
	exit 1
fi

portDirectory="$haikuportsTree/haiku-apps/fatcat"
mkdir -p "$portDirectory"
cp "$recipe" "$portDirectory/fatcat-$version.recipe"

echo "Installed recipe in $portDirectory"
(
	cd "$haikuportsTree"
	"$haikuporter" -S "$@" "fatcat-$version"
)

echo "Haiku package build complete. Packages are in $haikuportsTree/packages."
