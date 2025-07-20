#!/bin/bash

# Git workflow helper script for raytracer development

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}🔧 Raytracer Git Workflow Helper${NC}"
echo "================================="

# Function to show current status
show_status() {
    echo -e "\n${YELLOW}📊 Current Status:${NC}"
    git status --short
    echo ""
    git log --oneline -5
    echo ""
}

# Function to run tests before commit
run_tests() {
    echo -e "\n${YELLOW}🧪 Running tests...${NC}"
    if ./build.sh > /dev/null 2>&1; then
        echo -e "${GREEN}✅ Build successful${NC}"
        return 0
    else
        echo -e "${RED}❌ Build failed - fix errors before committing${NC}"
        return 1
    fi
}

# Parse command line arguments
case "${1:-status}" in
    "status"|"s")
        show_status
        ;;
    "commit"|"c")
        if [[ -z "$2" ]]; then
            echo -e "${RED}Usage: $0 commit \"commit message\"${NC}"
            exit 1
        fi
        
        echo -e "${YELLOW}🔍 Checking for changes...${NC}"
        if [[ -z $(git diff --cached --name-only) ]]; then
            echo -e "${RED}No staged changes found. Use 'git add' first.${NC}"
            exit 1
        fi
        
        if run_tests; then
            git commit -m "$2"
            echo -e "${GREEN}✅ Committed successfully${NC}"
        else
            echo -e "${RED}❌ Commit aborted due to test failure${NC}"
            exit 1
        fi
        ;;
    "feature"|"f")
        if [[ -z "$2" ]]; then
            echo -e "${RED}Usage: $0 feature \"feature-name\"${NC}"
            exit 1
        fi
        
        BRANCH_NAME="feature/$2"
        git checkout -b "$BRANCH_NAME"
        echo -e "${GREEN}✅ Created and switched to branch: $BRANCH_NAME${NC}"
        ;;
    "release"|"r")
        if [[ -z "$2" ]]; then
            echo -e "${RED}Usage: $0 release \"v1.1.0\"${NC}"
            exit 1
        fi
        
        echo -e "${YELLOW}🚀 Creating release $2...${NC}"
        if run_tests; then
            git tag -a "$2" -m "Release $2"
            echo -e "${GREEN}✅ Release tag $2 created${NC}"
        else
            echo -e "${RED}❌ Release aborted due to test failure${NC}"
            exit 1
        fi
        ;;
    "clean")
        echo -e "${YELLOW}🧹 Cleaning up...${NC}"
        git clean -fd
        rm -rf build/
        echo -e "${GREEN}✅ Repository cleaned${NC}"
        ;;
    "help"|"h"|*)
        echo -e "${YELLOW}Available commands:${NC}"
        echo "  status, s           - Show git status and recent commits"
        echo "  commit, c \"msg\"    - Run tests and commit if successful"
        echo "  feature, f \"name\"  - Create new feature branch"
        echo "  release, r \"v1.x\"  - Create release tag after testing"
        echo "  clean               - Clean repository and build files"
        echo "  help, h             - Show this help"
        echo ""
        echo -e "${YELLOW}Examples:${NC}"
        echo "  $0 status"
        echo "  $0 commit \"Add texture mapping support\""
        echo "  $0 feature \"volumetric-rendering\""
        echo "  $0 release \"v1.1.0\""
        ;;
esac
