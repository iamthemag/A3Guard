# Manual Workflow Trigger Guide

## Status

✅ Tag v1.0.0 is committed and pushed to GitHub
✅ release.yml workflow is configured
⏳ GitHub Actions jobs not starting automatically

## Solution

There are three ways to trigger the release workflow:

### Option 1: Wait for GitHub to Detect Tag (Automatic)
- GitHub sometimes takes a few minutes to detect new tags
- Wait 2-5 minutes and check: https://github.com/iamthemag/A3Guard/actions
- Refresh the page several times

### Option 2: Use GitHub CLI (Recommended - Fastest)

1. **Install GitHub CLI** (if not already installed):
   ```bash
   curl -fsSL https://cli.github.com/packages/githubcli-archive-keyring.gpg | sudo gpg --dearmor -o /usr/share/keyrings/githubcli-archive-keyring.gpg
   echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/githubcli-archive-keyring.gpg] https://cli.github.com/packages stable main" | sudo tee /etc/apt/sources.list.d/github-cli.list > /dev/null
   sudo apt update
   sudo apt install gh
   ```

2. **Login to GitHub** (if not already logged in):
   ```bash
   gh auth login
   # Select HTTPS, authenticate with personal access token
   ```

3. **Manually Trigger Release Workflow**:
   ```bash
   cd /home/test/A3Guard
   
   # Using workflow_dispatch to manually trigger
   gh workflow run release.yml \
     --repo iamthemag/A3Guard \
     --ref v1.0.0
   ```

4. **Monitor the workflow**:
   ```bash
   # Watch the workflow run in real-time
   gh run list --repo iamthemag/A3Guard --workflow release.yml --limit 1
   
   # Get detailed status
   gh run view <run-id> --repo iamthemag/A3Guard --log
   ```

### Option 3: Use Web Interface (Manual)

1. Go to: https://github.com/iamthemag/A3Guard/actions
2. Click on "Release - Build DEB Package" workflow
3. Click "Run workflow" button
4. Select branch: `main`
5. Click green "Run workflow" button

### Option 4: Re-push the Tag (Force Trigger)

If the tag still isn't triggering:

```bash
cd /home/test/A3Guard

# Delete tag locally and from remote
git tag -d v1.0.0
git push origin :refs/tags/v1.0.0

# Recreate and push
git tag -a v1.0.0 -m "A3Guard v1.0.0 Release"
git push origin v1.0.0

# Wait a few minutes for GitHub to detect
```

## What Should Happen

Once the workflow triggers:

1. ✅ Validate Release Tag job starts
2. ✅ Extract version: 1.0.0
3. ✅ Build Release Package job starts on ubuntu-20.04
4. ✅ Install dependencies
5. ✅ Build binary
6. ✅ Create DEB package with A3Guard.png icon
7. ✅ Verify package
8. ✅ Create GitHub Release
9. ✅ Upload DEB file to release
10. ✅ Security Scan job runs
11. ✅ Notify Success job runs

## Monitoring

**Watch the workflow in real-time:**
- GitHub Actions: https://github.com/iamthemag/A3Guard/actions
- Release page: https://github.com/iamthemag/A3Guard/releases/tag/v1.0.0

**Expected workflow duration:** 5-10 minutes

**After completion:**
- DEB file will be available: `a3guard_1.0.0_amd64.deb`
- Release notes auto-generated
- Icon included in package

## Troubleshooting

### Workflow still not showing up?
1. Clear browser cache: Ctrl+Shift+Delete
2. Refresh GitHub Actions page
3. Check if tag exists on GitHub: https://github.com/iamthemag/A3Guard/tags
4. Try Option 4 (re-push tag)

### Build fails after workflow starts?
- Check build logs in GitHub Actions
- Common issues:
  - Missing dependencies (should be installed automatically)
  - CMake errors (check CMakeLists.txt)
  - Source file issues (all files should be committed)

### Can't find DEB file after build?
- Check build logs for errors
- Verify CMakeLists.txt has correct CPACK configuration
- Check if DEB was created: look in "Build Release Package" job logs

## Expected Output Files

After successful build:
- Binary: `/opt/a3guard/bin/A3Guard`
- DEB: `a3guard_1.0.0_amd64.deb`
- Icon: `/usr/share/pixmaps/A3Guard.png`

## Commands Quick Reference

```bash
# Check tag locally
cd /home/test/A3Guard
git tag -l -n1 v1.0.0

# Check if tag is on GitHub
git ls-remote --tags origin v1.0.0

# View workflow file
cat .github/workflows/release.yml

# Trigger with GitHub CLI
gh workflow run release.yml --ref v1.0.0

# Check workflow runs
gh run list --workflow release.yml
```

## Next Steps

1. Try Option 2 (GitHub CLI) - it's the fastest
2. Or wait 5 minutes and refresh GitHub Actions page
3. If neither works, try Option 4 (re-push tag)
4. If workflow still fails, check build logs for specific errors

The release should be ready once the workflow completes successfully! 🚀
