#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build"
OUTPUT_DIR="dist"
CONFIGURATION="Release"
VERSION="1.0.0"
EDITION="Public"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-dir) BUILD_DIR="$2"; shift 2 ;;
    --output-dir) OUTPUT_DIR="$2"; shift 2 ;;
    --configuration) CONFIGURATION="$2"; shift 2 ;;
    --version) VERSION="$2"; shift 2 ;;
    --edition) EDITION="$2"; shift 2 ;;
    *) echo "Unknown argument: $1" >&2; exit 2 ;;
  esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

case "$EDITION" in
  Public)
    PRODUCT_NAME="WaviateScript"
    PACKAGE_STEM="WaviateScript"
    PKG_IDENTIFIER="com.yourcompany.WaviateScript.pkg"
    ;;
  Premium)
    PRODUCT_NAME="WaviateScript Premium"
    PACKAGE_STEM="WaviateScript-Premium"
    PKG_IDENTIFIER="com.yourcompany.WaviateScript.pkg"
    ;;
  *)
    echo "--edition must be Public or Premium" >&2
    exit 2
    ;;
esac

if [[ "$BUILD_DIR" != /* ]]; then
  BUILD_DIR="$REPO_ROOT/$BUILD_DIR"
fi

if [[ "$OUTPUT_DIR" != /* ]]; then
  OUTPUT_DIR="$REPO_ROOT/$OUTPUT_DIR"
fi

find_first() {
  for candidate in "$@"; do
    if [[ -e "$candidate" ]]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done
  return 1
}

STANDALONE_APP="$(find_first \
  "$BUILD_DIR/WaviateScript_artefacts/$CONFIGURATION/Standalone/WaviateScript.app" \
  "$BUILD_DIR/WaviateScript_artefacts/Standalone/WaviateScript.app" \
  "$BUILD_DIR/WaviateScript_artefacts/$CONFIGURATION/Standalone Plugin/WaviateScript.app" \
  "$BUILD_DIR/WaviateScript_artefacts/Standalone Plugin/WaviateScript.app" \
  || true)"

VST3_BUNDLE="$(find_first \
  "$BUILD_DIR/WaviateScript_artefacts/$CONFIGURATION/VST3/WaviateScript.vst3" \
  "$BUILD_DIR/WaviateScript_artefacts/VST3/WaviateScript.vst3" \
  "$BUILD_DIR/$CONFIGURATION/VST3/WaviateScript.vst3" \
  "$BUILD_DIR/VST3/WaviateScript.vst3" \
  || true)"

if [[ -z "$STANDALONE_APP" ]]; then
  echo "Standalone .app not found under '$BUILD_DIR'. Build with CMake first." >&2
  exit 1
fi

if [[ -z "$VST3_BUNDLE" ]]; then
  echo "VST3 bundle not found under '$BUILD_DIR'. Build with CMake first." >&2
  exit 1
fi

if ! command -v pkgbuild >/dev/null 2>&1; then
  echo "pkgbuild was not found. Install Xcode command line tools first." >&2
  exit 1
fi

mkdir -p "$OUTPUT_DIR"
WORK_DIR="$OUTPUT_DIR/_pkg-$PACKAGE_STEM"
STAGING="$WORK_DIR/root"
SCRIPTS="$WORK_DIR/scripts"
COMPONENT_PKG="$WORK_DIR/$PACKAGE_STEM-component.pkg"
OUTPUT_PKG="$OUTPUT_DIR/$PACKAGE_STEM-$VERSION-macos.pkg"

rm -rf "$WORK_DIR"
mkdir -p "$STAGING/Applications" "$STAGING/Library/Audio/Plug-Ins/VST3" "$SCRIPTS"

ditto "$STANDALONE_APP" "$STAGING/Applications/WaviateScript.app"
ditto "$VST3_BUNDLE" "$STAGING/Library/Audio/Plug-Ins/VST3/WaviateScript.vst3"

PKGBUILD_ARGS=(
  --root "$STAGING"
  --identifier "$PKG_IDENTIFIER"
  --version "$VERSION"
  --install-location "/"
)

if [[ -n "${MACOS_INSTALLER_SIGN_IDENTITY:-}" ]]; then
  PKGBUILD_ARGS+=(--sign "$MACOS_INSTALLER_SIGN_IDENTITY")
fi

PKGBUILD_ARGS+=("$COMPONENT_PKG")

echo "Packaging $PRODUCT_NAME"
echo "  Standalone: $STANDALONE_APP"
echo "  VST3:       $VST3_BUNDLE"
echo "  Output:     $OUTPUT_PKG"

pkgbuild "${PKGBUILD_ARGS[@]}"

if command -v productbuild >/dev/null 2>&1; then
  PRODUCTBUILD_ARGS=(--package "$COMPONENT_PKG")
  if [[ -n "${MACOS_INSTALLER_SIGN_IDENTITY:-}" ]]; then
    PRODUCTBUILD_ARGS+=(--sign "$MACOS_INSTALLER_SIGN_IDENTITY")
  fi
  PRODUCTBUILD_ARGS+=("$OUTPUT_PKG")
  productbuild "${PRODUCTBUILD_ARGS[@]}"
else
  cp "$COMPONENT_PKG" "$OUTPUT_PKG"
fi

if [[ ! -f "$OUTPUT_PKG" ]]; then
  echo "pkg was not created: $OUTPUT_PKG" >&2
  exit 1
fi

echo "Created installer: $OUTPUT_PKG"
