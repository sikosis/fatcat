#!/bin/sh

set -eu

scriptDirectory=$(CDPATH= cd "$(dirname "$0")" && pwd)
projectDirectory=$(CDPATH= cd "$scriptDirectory/.." && pwd)
prepareOnly=false
bootstrap=false

while [ $# -gt 0 ]; do
	case "$1" in
		--prepare-only)
			prepareOnly=true
			shift
			;;
		--bootstrap)
			bootstrap=true
			shift
			;;
		--)
			shift
			break
			;;
		*)
			break
			;;
	esac
done

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
haikuportsConfiguration="${HOME}/config/settings/haikuports.conf"
if [ -z "$haikuportsTree" ]; then
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

if [ -z "$haikuportsTree" ]; then
	for candidate in "$HOME/haikuports" "$HOME/src/haikuports" \
		/boot/home/haikuports; do
		if [ -d "$candidate" ]; then
			haikuportsTree=$candidate
			break
		fi
	done
fi

confirmBootstrap()
{
	description=$1
	destination=$2

	if [ "$bootstrap" = true ]; then
		return 0
	fi
	if [ ! -t 0 ]; then
		return 1
	fi

	printf '%s is required. Clone it to %s? [Y/n] ' "$description" "$destination"
	read -r answer
	case "$answer" in
		""|y|Y|yes|YES|Yes)
			return 0
			;;
		*)
			return 1
			;;
	esac
}

if [ -z "$haikuportsTree" ]; then
	haikuportsTree="$HOME/haikuports"
fi

if [ ! -d "$haikuportsTree" ]; then
	if ! command -v git >/dev/null 2>&1; then
		echo "Git is required to download HaikuPorts. Install it with pkgman." >&2
		exit 1
	fi
	if ! confirmBootstrap "A HaikuPorts checkout" "$haikuportsTree"; then
		echo "Set HAIKUPORTS_TREE to an existing checkout, or use --bootstrap." >&2
		exit 1
	fi
	mkdir -p "$(dirname "$haikuportsTree")"
	echo "Cloning the official HaikuPorts tree..."
	git clone --depth=1 https://github.com/haikuports/haikuports.git \
		"$haikuportsTree"
fi

haikuporter=${HAIKUPORTER:-}
if [ -z "$haikuporter" ] && command -v haikuporter >/dev/null 2>&1; then
	haikuporter=$(command -v haikuporter)
fi
if [ -z "$haikuporter" ]; then
	for candidate in "$HOME/haikuporter/haikuporter" \
		"$(dirname "$haikuportsTree")/haikuporter/haikuporter" \
		/boot/home/haikuporter/haikuporter; do
		if [ -x "$candidate" ]; then
			haikuporter=$candidate
			break
		fi
	done
fi

if [ -z "$haikuporter" ] || [ ! -x "$haikuporter" ]; then
	haikuporter="$HOME/haikuporter/haikuporter"
	if ! command -v git >/dev/null 2>&1; then
		echo "Git is required to download HaikuPorter. Install it with pkgman." >&2
		exit 1
	fi
	if ! confirmBootstrap "HaikuPorter" "$(dirname "$haikuporter")"; then
		echo "Set HAIKUPORTER to its executable, or use --bootstrap." >&2
		exit 1
	fi
	echo "Cloning the official HaikuPorter tool..."
	git clone --depth=1 https://github.com/haikuports/haikuporter.git \
		"$(dirname "$haikuporter")"
fi

buildConfiguration="$outputDirectory/haikuports.conf"
if [ -f "$haikuportsConfiguration" ]; then
	awk -v tree="$haikuportsTree" '
		BEGIN { found = 0 }
		/^[[:space:]]*TREE_PATH=/ {
			print "TREE_PATH=\"" tree "\""
			found = 1
			next
		}
		{ print }
		END {
			if (!found)
				print "TREE_PATH=\"" tree "\""
		}' "$haikuportsConfiguration" > "$buildConfiguration"
else
	packager=${FATCAT_PACKAGER:-"Fat Cat local builder <fatcat@localhost>"}
	{
		printf 'TREE_PATH="%s"\n' "$haikuportsTree"
		printf 'PACKAGER="%s"\n' "$packager"
	} > "$buildConfiguration"
fi

portDirectory="$haikuportsTree/haiku-apps/fatcat"
mkdir -p "$portDirectory"
cp "$recipe" "$portDirectory/fatcat-$version.recipe"

echo "Installed recipe in $portDirectory"
(
	cd "$haikuportsTree"
	"$haikuporter" --config="$buildConfiguration" -S "$@" "fatcat-$version"
)

echo "Haiku package build complete. Packages are in $haikuportsTree/packages."
