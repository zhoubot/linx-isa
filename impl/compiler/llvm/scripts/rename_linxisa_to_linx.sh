#!/bin/bash
# Script to rename "LinxISA" to "Linx" throughout LLVM backend code
# Usage: ./rename_linxisa_to_linx.sh <llvm-project-path>

set -euo pipefail

if [ $# -ne 1 ]; then
    echo "Usage: $0 <llvm-project-path>"
    echo "Example: $0 ~/llvm-project"
    exit 1
fi

LLVM_PROJECT="$1"
TARGET_DIR="$LLVM_PROJECT/llvm/lib/Target/Linx"

if [ ! -d "$TARGET_DIR" ]; then
    echo "Error: Target directory not found: $TARGET_DIR"
    echo "Note: Directory might be named 'LinxISA' - rename it first"
    exit 1
fi

echo "Renaming 'LinxISA' to 'Linx' in: $TARGET_DIR"
echo ""

# Find all files in the target directory
find "$TARGET_DIR" -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.td" -o -name "*.def" \) | while read -r file; do
    echo "Processing: $file"
    
    # Replace LinxISA with Linx in file contents
    sed -i.bak 's/LinxISA/Linx/g' "$file"
    sed -i.bak 's/linxisa/linx/g' "$file"
    sed -i.bak 's/LINXISA/LINX/g' "$file"
    
    # Remove backup files
    rm -f "${file}.bak"
done

# Rename files
find "$TARGET_DIR" -type f -name "*LinxISA*" | while read -r file; do
    newname=$(echo "$file" | sed 's/LinxISA/Linx/g')
    echo "Renaming: $file -> $newname"
    mv "$file" "$newname"
done

# Rename directories
find "$TARGET_DIR" -type d -name "*LinxISA*" | sort -r | while read -r dir; do
    newname=$(echo "$dir" | sed 's/LinxISA/Linx/g')
    echo "Renaming directory: $dir -> $newname"
    mv "$dir" "$newname"
done

echo ""
echo "Done! Please review changes and update CMakeLists.txt if needed."
