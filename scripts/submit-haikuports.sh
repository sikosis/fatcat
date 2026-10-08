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

checksumFile()
{
	if command -v sha256sum >/dev/null 2>&1; then
		sha256sum "$1" | awk '{print $1}'
	elif command -v sha256 >/dev/null 2>&1; then
		sha256 "$1" | awk '{print $NF}'
	elif command -v shasum >/dev/null 2>&1; then
		shasum -a 256 "$1" | awk '{print $1}'
	else
		echo "A SHA-256 utility (sha256sum, sha256, or shasum) is required." >&2
		exit 1
	fi
}

archiveIsValid()
{
	[ -s "$1" ] || return 1
	tar -tzf "$1" >/dev/null 2>&1 || return 1
	archiveRoot=$(tar -tzf "$1" 2>/dev/null | sed -n '1p')
	[ "$archiveRoot" = "fatcat-$releaseVersion/" ]
}

downloadWithCurl()
{
	attempt=1
	while [ "$attempt" -le 4 ]; do
		echo "Downloading tagged source with curl (attempt $attempt of 4) ..."
		if curl --fail --location --connect-timeout 15 --max-time 300 \
			--output "$temporaryArchive" "$sourceUri"; then
			return 0
		fi
		attempt=$((attempt + 1))
		if [ "$attempt" -le 4 ]; then
			sleep 2
		fi
	done
	return 1
}

downloadWithWget()
{
	echo "curl could not download the archive; trying wget ..."
	wget --timeout=15 --tries=4 --output-document="$temporaryArchive" \
		"$sourceUri"
}

mkdir -p "$outputDirectory"
useCachedArchive=false
if [ -f "$archive" ] && [ -f "$publicRecipe" ]; then
	cachedSourceUri=$(sed -n 's/^SOURCE_URI="\([^"]*\)"/\1/p' \
		"$publicRecipe")
	cachedChecksum=$(sed -n 's/^CHECKSUM_SHA256="\([^"]*\)"/\1/p' \
		"$publicRecipe")
	actualChecksum=$(checksumFile "$archive")
	if [ "$cachedSourceUri" = "$sourceUri" ] \
		&& [ "$cachedChecksum" = "$actualChecksum" ] \
		&& archiveIsValid "$archive"; then
		useCachedArchive=true
		checksum=$actualChecksum
		echo "Using verified cached tagged source archive: $archive"
	fi
fi

if [ "$useCachedArchive" = false ]; then
	if ! git -C "$projectDirectory" ls-remote --exit-code --tags origin \
		"refs/tags/$tag" >/dev/null 2>&1; then
		echo "The tag $tag could not be verified on the Fat Cat origin remote." >&2
		echo "Check the network connection and confirm the tag was pushed." >&2
		exit 1
	fi

	temporaryArchive="$archive.download.$$"
	trap 'rm -f "$temporaryArchive"' EXIT HUP INT TERM
	downloaded=false
	if command -v curl >/dev/null 2>&1 && downloadWithCurl; then
		downloaded=true
	elif command -v wget >/dev/null 2>&1 && downloadWithWget; then
		downloaded=true
	fi

	if [ "$downloaded" = false ]; then
		echo "Unable to download $sourceUri after retries." >&2
		echo "No verified cached archive was available; check DNS/network access and retry." >&2
		exit 1
	fi
	if ! archiveIsValid "$temporaryArchive"; then
		echo "The downloaded archive is invalid or has an unexpected top-level directory." >&2
		exit 1
	fi

	checksum=$(checksumFile "$temporaryArchive")
	mv "$temporaryArchive" "$archive"
	trap - EXIT HUP INT TERM
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

if ! command -v curl >/dev/null 2>&1; then
	echo "curl is required for GitHub API access." >&2
	exit 1
fi

githubToken=${GITHUB_TOKEN:-}
if [ -z "$githubToken" ]; then
	if [ ! -t 0 ]; then
		echo "Set GITHUB_TOKEN to a token with the public_repo scope." >&2
		exit 1
	fi
	while [ -z "$githubToken" ]; do
		echo "Paste the GitHub token once, then press Enter. Input remains hidden."
		printf 'Token: '
		trap 'stty echo; exit 1' HUP INT TERM
		stty -echo
		if ! read -r githubToken; then
			stty echo
			trap - HUP INT TERM
			echo
			exit 1
		fi
		tokenSuffix=$(printf '%s\n' "$githubToken" | sed 's/^.*\(....\)$/\1/')
		printf '\nToken captured (ending in %s).\n' "$tokenSuffix"
		printf 'Press Enter to validate, or paste a corrected token and press Enter: '
		if ! read -r confirmation; then
			stty echo
			trap - HUP INT TERM
			echo
			exit 1
		fi
		stty echo
		trap - HUP INT TERM
		printf '\n'

		case "$confirmation" in
			"")
				;;
			ghp_*|github_pat_*)
				githubToken=$confirmation
				tokenSuffix=$(printf '%s\n' "$githubToken" \
					| sed 's/^.*\(....\)$/\1/')
				echo "Using the corrected token ending in $tokenSuffix."
				;;
			*)
				echo "Input was not a recognised token; please try again." >&2
				githubToken=
				continue
				;;
		esac

		case "$githubToken" in
			ghp_*ghp_*|github_pat_*github_pat_*)
				echo "The token appears to have been pasted twice; please try again." >&2
				githubToken=
				;;
		esac
	done
fi
if [ -z "$githubToken" ]; then
	echo "A GitHub personal access token is required." >&2
	exit 1
fi

githubApiGet()
{
	curl --fail --silent --show-error \
		--header "Accept: application/vnd.github+json" \
		--header "Authorization: Bearer $githubToken" \
		--header "X-GitHub-Api-Version: 2022-11-28" \
		"$1"
}

githubApiPost()
{
	curl --fail --silent --show-error \
		--request POST \
		--header "Accept: application/vnd.github+json" \
		--header "Authorization: Bearer $githubToken" \
		--header "X-GitHub-Api-Version: 2022-11-28" \
		--header "Content-Type: application/json" \
		--data-binary "@$2" \
		"$1"
}

if ! userResponse=$(githubApiGet "https://api.github.com/user" 2>/dev/null); then
	echo "GitHub rejected that token (HTTP 401)." >&2
	echo "Create a valid classic personal access token with public_repo scope." >&2
	exit 1
fi
githubUser=$(printf '%s\n' "$userResponse" | sed -n \
	's/^[[:space:]]*"login": "\([^"]*\)",*/\1/p' | head -n 1)
if [ -z "$githubUser" ]; then
	echo "Could not determine the GitHub user for the supplied token." >&2
	exit 1
fi

echo "Authenticated with GitHub as $githubUser."

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

gitName=$(git -C "$projectDirectory" config user.name || true)
gitEmail=$(git -C "$projectDirectory" config user.email || true)
if [ -z "$gitName" ] || [ -z "$gitEmail" ]; then
	packager=$(awk -F= '
		/^[[:space:]]*PACKAGER=/ {
			value = substr($0, index($0, "=") + 1)
			gsub(/^[[:space:]\"]+|[[:space:]\"]+$/, "", value)
			print value
			exit
		}' "$buildConfiguration")
	derivedName=$(printf '%s\n' "$packager" \
		| sed -n 's/^\(.*\) <[^>]*>$/\1/p')
	derivedEmail=$(printf '%s\n' "$packager" \
		| sed -n 's/^.*<\([^>]*\)>$/\1/p')
	gitName=${gitName:-$derivedName}
	gitEmail=${gitEmail:-$derivedEmail}
fi
if [ -z "$gitName" ] || [ -z "$gitEmail" ]; then
	echo "Set PACKAGER in haikuports.conf or configure git user.name and user.email." >&2
	exit 1
fi

git -C "$submissionTree" config user.name "$gitName"
git -C "$submissionTree" config user.email "$gitEmail"

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

git -C "$submissionTree" add \
	"haiku-apps/fatcat/fatcat-$releaseVersion.recipe"
git -C "$submissionTree" commit -m \
	"haiku-apps/fatcat: add $releaseVersion"

forkResponse="$submissionDirectory/github-fork.json"
if ! githubApiGet "https://api.github.com/repos/$githubUser/haikuports" \
	> "$forkResponse" 2>/dev/null; then
	echo "Creating $githubUser/haikuports fork..."
	forkRequest="$submissionDirectory/github-fork-request.json"
	printf '{"default_branch_only":true}\n' > "$forkRequest"
	githubApiPost "https://api.github.com/repos/haikuports/haikuports/forks" \
		"$forkRequest" > "$forkResponse"

	forkReady=false
	attempt=0
	while [ "$attempt" -lt 30 ]; do
		if githubApiGet "https://api.github.com/repos/$githubUser/haikuports" \
			> "$forkResponse" 2>/dev/null; then
			forkReady=true
			break
		fi
		attempt=$((attempt + 1))
		sleep 2
	done
	if [ "$forkReady" != true ]; then
		echo "The HaikuPorts fork was created but is not ready yet." >&2
		echo "Wait a minute, then rerun the submission command." >&2
		exit 1
	fi
fi

existingPulls="$submissionDirectory/github-pulls.json"
githubApiGet "https://api.github.com/repos/haikuports/haikuports/pulls?state=open&head=$githubUser:$branch" \
	> "$existingPulls"
if grep -q '"html_url"' "$existingPulls"; then
	echo "A HaikuPorts pull request already exists for $githubUser:$branch." >&2
	exit 1
fi

askpass="$submissionDirectory/git-askpass.sh"
printf '%s\n' \
	'#!/bin/sh' \
	'case "$1" in' \
	'  *Username*) printf "%s\\n" "x-access-token" ;;' \
	'  *) printf "%s\\n" "$GITHUB_TOKEN" ;;' \
	'esac' > "$askpass"
chmod 700 "$askpass"

git -C "$submissionTree" remote add fork \
	"https://github.com/$githubUser/haikuports.git"
GITHUB_TOKEN=$githubToken GIT_ASKPASS=$askpass GIT_TERMINAL_PROMPT=0 \
	git -C "$submissionTree" push --set-upstream fork "$branch"

pullRequest="$submissionDirectory/github-pull-request.json"
printf '{"title":"haiku-apps/fatcat: add %s","head":"%s:%s","base":"master","body":"Adds Fat Cat Pomodoro %s, a native Haiku Pomodoro timer with a Deskbar add-on and animated cat break overlays. Built and tested locally on Haiku x86_64."}\n' \
	"$releaseVersion" "$githubUser" "$branch" "$releaseVersion" \
	> "$pullRequest"
pullResponse="$submissionDirectory/github-pull-response.json"
githubApiPost "https://api.github.com/repos/haikuports/haikuports/pulls" \
	"$pullRequest" > "$pullResponse"
pullRequestUrl=$(sed -n \
	's/^[[:space:]]*"html_url": "\([^"]*\)",*/\1/p' "$pullResponse" | head -n 1)

releaseResponse="$submissionDirectory/github-release.json"
if ! githubApiGet \
	"https://api.github.com/repos/$projectRepository/releases/tags/$tag" \
	> "$releaseResponse" 2>/dev/null; then
	releaseRequest="$submissionDirectory/github-release-request.json"
	printf '{"tag_name":"%s","name":"Fat Cat Pomodoro %s","body":"Fat Cat Pomodoro %s for Haiku."}\n' \
		"$tag" "$tag" "$tag" > "$releaseRequest"
	githubApiPost "https://api.github.com/repos/$projectRepository/releases" \
		"$releaseRequest" > "$releaseResponse"
fi

releaseId=$(sed -n \
	's/^[[:space:]]*"id": \([0-9]*\),*/\1/p' "$releaseResponse" | head -n 1)
if [ -z "$releaseId" ]; then
	echo "Could not determine the GitHub release ID for $tag." >&2
	exit 1
fi

packageName=$(basename "$package")
if grep -F "\"name\": \"$packageName\"" \
	"$releaseResponse" >/dev/null 2>&1; then
	echo "Release asset already exists; leaving it unchanged: $packageName"
else
	uploadResponse="$submissionDirectory/github-upload.json"
	curl --fail --silent --show-error \
		--request POST \
		--header "Accept: application/vnd.github+json" \
		--header "Authorization: Bearer $githubToken" \
		--header "X-GitHub-Api-Version: 2022-11-28" \
		--header "Content-Type: application/octet-stream" \
		--data-binary "@$package" \
		"https://uploads.github.com/repos/$projectRepository/releases/$releaseId/assets?name=$packageName" \
		> "$uploadResponse"
fi

echo "HaikuPorts pull request: $pullRequestUrl"
echo "Validated package: $package"
echo "Submission checkout retained at: $submissionDirectory"
