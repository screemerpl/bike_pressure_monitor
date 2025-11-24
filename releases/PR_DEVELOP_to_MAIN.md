# PR: develop → main

Title: Release v0.5.0 - Merge develop into main

## Summary
This PR merges `develop` branch into `main` as part of the v0.5.0 release.

- Includes v0.5.0 release changes.
- Please ensure all CI checks pass before merging to `main`.

## Changelog
See `CHANGELOG.md` under root for release notes.

## Release Actions
- Annotated tag: `v0.5.0` created on `release/0.5.0` branch.
- After merging into `main`, create a GitHub Release and attach `CHANGELOG.md`, link to `v0.5.0` tag.

## Testing
- Build locally: `idf.py build` succeeded for `v0.5.0`.
- On device: flash and test both modes, pairing, web UI.

## Approvers
- Team leads: @screemerpl

---

(You can copy this content into the GitHub PR description when creating the PR.)