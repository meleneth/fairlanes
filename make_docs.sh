#!/usr/bin/env bash
set -euo pipefail

# Default paths
SRC_DIR="${1:-src}"
OUT_DIR="docs"

# Generate a minimal Doxyfile if missing
if [[ ! -f Doxyfile ]]; then
  cat > Doxyfile <<'EOF'
# Basic Doxygen config
PROJECT_NAME           = "MyProject"
OUTPUT_DIRECTORY       = docs
GENERATE_HTML          = YES
GENERATE_LATEX         = NO
RECURSIVE              = YES
QUIET                  = NO

INPUT                  = src include
FILE_PATTERNS          = *.h *.hpp *.cpp *.cc *.cxx
EXCLUDE_PATTERNS       = */build/* */third_party/* */extern/*

EXTRACT_PRIVATE        = NO
EXTRACT_STATIC         = NO
EXTRACT_LOCAL_CLASSES  = YES
INLINE_SOURCES         = YES

# Source browser and call graphs (optional)
SOURCE_BROWSER         = YES
CALL_GRAPH             = NO
CALLER_GRAPH           = NO

# Markdown & docs tweaks
USE_MDFILE_AS_MAINPAGE = README.md
MARKDOWN_SUPPORT       = YES
FULL_PATH_NAMES        = NO
STRIP_FROM_PATH        = .

# Set language and encoding
OPTIMIZE_OUTPUT_FOR_C  = NO
OPTIMIZE_OUTPUT_JAVA   = NO
OPTIMIZE_OUTPUT_VHDL   = NO
OPTIMIZE_OUTPUT_SLICE  = NO
OPTIMIZE_OUTPUT_FOR_CPLUSPLUS = YES

# Exclude system includes
SEARCH_INCLUDES        = NO
EOF
  echo "Generated default Doxyfile."
fi

# Run Doxygen
echo "Generating docs..."
doxygen Doxyfile

# Print result
echo
echo "HTML docs generated in: $(realpath "$OUT_DIR/html")"
