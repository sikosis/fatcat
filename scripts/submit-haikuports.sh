#!/bin/sh

set -eu

scriptDirectory=$(CDPATH= cd "$(dirname "$0")" && pwd)
projectDirectory=$(CDPATH= cd "$scriptDirectory/.." && pwd)
prepareOnly=false

if [ "${1:-}" = "--prepare-only" ]; then
	prepareOnly=true
	shift
fi

releaseVersion=${1:-}
projectRepository=${FATCAT_GITHUB_REPOSITORY:-sikosis/fatcat}

if [ -z "$releaseVersion" ]; then
	echo "Usage: $0 [--prepare-only] VERSION" >&2
	echo "Example: $0 0.17" >&2
	exit 1
fi

case "$releaseVersion" in
	*[!0-9.]*|.*|*.)
		echo "Invalid release version: $releaseVersion" >&2
		exit 1
		;;
esac

tag="v$releaseVersion"
branch="fatcat-$releaseVersion"
recipeTemplate="$projectDirectory/packaging/haikuports/fatcat-$(sed -n \
	's/^constexpr const char\* kAppVersion = "\([^"]*\)";/\1/p' \
	"$projectDirectory/src/Messages.h").recipe.in"
outputDirectory="$projectDirectory/dist"
archiveName="fatcat-$releaseVersion.tar.gz"
archive="$outputDirectory/$archiveName"
publicRecipe="$outputDirectory/fatcat-$releaseVersion.haikuports.recipe"
sourceUri="https://github.com/$projectRepository/archive/refs/tags/$tag.tar.gz"

for commandName in git awk sed tar; do
	if ! command -v "$commandName" >/dev/null 2>&1; then
		echo "Required command not found: $commandName" >&2
		exit 1
	fi
done

if [ ! -f "$recipeTemplate" ]; then
	echo "Missing current recipe template: $recipeTemplate" >&2
	exit 1
fi

if ! git -C "$projectDirectory" ls-remote --exit-code --tags origin \
	"refs/tags/$tag" >/dev/null 2>&1; then
	echo "The tag $tag is not available from the Fat Cat origin remote." >&2
	echo "Push the tag before submitting it to HaikuPorts." >&2
	exit 1
fi

mkdir -p "$outputDirectory"
if command -v curl >/dev/null 2>&1; then
	curl --fail --location --output "$archive" "$sourceUri"
elif command -v wget >/dev/null 2>&1; then
	wget --output-document="$archive" "$sourceUri"
else
	echo "curl or wget is required to download the tagged source archive." >&2
	exit 1
fi

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

escapedHomepage=$(escapeReplacement "https://github.com/$projectRepository")
escapedSourceUri=$(escapeReplacement "$sourceUri")
escapedChecksum=$(escapeReplacement "$checksum")
escapedArchiveName=$(escapeReplacement "$archiveName")
escapedSourceDirectory=$(escapeReplacement "fatcat-$releaseVersion")

sed \
	-e "s|@HOMEPAGE@|$escapedHomepage|g" \
	-e "s|@SOURCE_URI@|$escapedSourceUri|g" \
	-e "s|@CHECKSUM_SHA256@|$escapedChecksum|g" \
	-e "s|@SOURCE_FILENAME@|$escapedArchiveName|g" \
	-e "s|@SOURCE_DIR@|$escapedSourceDirectory|g" \
	"$recipeTemplate" > "$publicRecipe"

if grep -E '@[A-Z_]+@' "$publicRecipe" >/dev/null 2>&1; then
	echo "The generated public recipe still contains placeholders." >&2
	exit 1
fi

echo "Created public recipe: $publicRecipe"
echo "Tagged source checksum: $checksum"

if [ "$prepareOnly" = true ]; then
	exit 0
fi

if ! command -v gh >/dev/null 2>&1; then
	echo "GitHub CLI is required. Install gh and rerun this command." >&2
	exit 1
fi
if ! gh auth status >/dev/null 2>&1; then
	echo "GitHub CLI is not authenticated. Run: gh auth login" >&2
	exit 1
fi

haikuporter=${HAIKUPORTER:-}
if [ -z "$haikuporter" ] && command -v haikuporter >/dev/null 2>&1; then
	haikuporter=$(command -v haikuporter)
fi
if [ -z "$haikuporter" ]; then
	for candidate in "$HOME/haikuporter/haikuporter" \
		/boot/home/haikuporter/haikuporter; do
		if [ -x "$candidate" ]; then
			haikuporter=$candidate
			break
		fi
	done
fi
if [ -z "$haikuporter" ] || [ ! -x "$haikuporter" ]; then
	echo "HaikuPorter was not found. Set HAIKUPORTER to its executable." >&2
	exit 1
fi

submissionDirectory=$(mktemp -d \
	"$outputDirectory/haikuports-submit-$releaseVersion.XXXXXX")
submissionTree="$submissionDirectory/haikuports"
echo "Cloning an isolated HaikuPorts tree into $submissionTree"
git clone --depth=1 https://github.com/haikuports/haikuports.git \
	"$submissionTree"
git -C "$submissionTree" switch -c "$branch"

portDirectory="$submissionTree/haiku-apps/fatcat"
mkdir -p "$portDirectory"
cp "$publicRecipe" "$portDirectory/fatcat-$releaseVersion.recipe"

haikuportsConfiguration="${HOME}/config/settings/haikuports.conf"
buildConfiguration="$submissionDirectory/haikuports.conf"
if [ -f "$haikuportsConfiguration" ]; then
	awk -v tree="$submissionTree" '
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
	packager=${FATCAT_PACKAGER:-"Sikosis <phil@sikosis.com>"}
	{
		printf 'TREE_PATH="%s"\n' "$submissionTree"
		printf 'PACKAGER="%s"\n' "$packager"
	} > "$buildConfiguration"
fi

echo "Validating the public recipe with HaikuPorter..."
(
	cd "$submissionTree"
	"$haikuporter" --config="$buildConfiguration" -S \
		"fatcat-$releaseVersion"
)

package=$(find "$submissionTree/packages" -type f \
	-name "fatcat-$releaseVersion-*.hpkg" -print | head -n 1)
if [ -z "$package" ]; then
	echo "HaikuPorter completed without producing the expected package." >&2
	exit 1
fi

gitName=$(git -C "$projectDirectory" config user.name || true)
gitEmail=$(git -C "$projectDirectory" config user.email || true)
if [ -z "$gitName" ] || [ -z "$gitEmail" ]; then
	echo "Configure git user.name and user.email before submitting." >&2
	exit 1
fi

git -C "$submissionTree" config user.name "$gitName"
git -C "$submissionTree" config user.email "$gitEmail"
git -C "$submissionTree" add \
	"haiku-apps/fatcat/fatcat-$releaseVersion.recipe"
git -C "$submissionTree" commit -m \
	"haiku-apps/fatcat: add $releaseVersion"

githubUser=$(gh api user --jq .login)
if ! gh repo view "$githubUser/haikuports" >/dev/null 2>&1; then
	echo "Creating $githubUser/haikuports fork..."
	gh repo fork haikuports/haikuports --clone=false
fi

if gh pr list --repo haikuports/haikuports --head "$githubUser:$branch" \
	--json url --jq '.[0].url' | grep -q .; then
	echo "A HaikuPorts pull request already exists for $githubUser:$branch." >&2
	exit 1
fi

gh auth setup-git
git -C "$submissionTree" remote add fork \
	"https://github.com/$githubUser/haikuports.git"
git -C "$submissionTree" push --set-upstream fork "$branch"

pullRequestUrl=$(gh pr create \
	--repo haikuports/haikuports \
	--base master \
	--head "$githubUser:$branch" \
	--title "haiku-apps/fatcat: add $releaseVersion" \
	--body "Adds Fat Cat Pomodoro $releaseVersion, a native Haiku Pomodoro timer with a Deskbar add-on and animated cat break overlays. Built and tested locally on Haiku x86_64.")

if ! gh release view "$tag" --repo "$projectRepository" >/dev/null 2>&1; then
	gh release create "$tag" \
		--repo "$projectRepository" \
		--verify-tag \
		--title "Fat Cat Pomodoro $tag" \
		--notes "Fat Cat Pomodoro $tag for Haiku."
fi

packageName=$(basename "$package")
if gh release view "$tag" --repo "$projectRepository" \
	--json assets --jq '.assets[].name' | grep -Fxq "$packageName"; then
	echo "Release asset already exists; leaving it unchanged: $packageName"
else
	gh release upload "$tag" "$package" --repo "$projectRepository"
fi

echo "HaikuPorts pull request: $pullRequestUrl"
echo "Validated package: $package"
echo "Submission checkout retained at: $submissionDirectory"
