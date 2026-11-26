# Publishing this repository on GitHub

Before making the repository public:

1. Replace the `UTERUS Project Contributors` copyright placeholder in `LICENSE` with the actual copyright holder(s), or confirm that the collective attribution is accurate.
2. Confirm that you own the rights to license the original firmware and documentation under MIT.
3. Review every file for passwords, personal contact information, tokens and hardware secrets. None are necessary to upload this repository.
4. Do not add third-party anatomical/cadaveric images, dataset frames, licensed photos, or AI illustrations without separate rights and clear attribution.
5. Run `bash tests/run_tests.sh` locally, upload the Arduino firmware with pumps **disconnected**, and test the hardware in a controlled environment.
6. Create a new GitHub repository with a suitable name (for example `uterus-interactive-pneumatics`) and upload the **contents of this folder**. Do not create a second GitHub-generated README or LICENSE that duplicates these files.
7. Check the repository's **Actions** tab for the firmware CI jobs. An AVR compilation check runs on GitHub's hosted runner; compilation is not the same as full pneumatic hardware validation.
8. After publishing, tag a release and add the real repository URL to academic materials and citations.

## Account security

**Never share your GitHub password, two-factor recovery codes, access tokens or private SSH keys in messages.** Use GitHub's own login/authorization flow, GitHub Desktop, or a trusted connected GitHub integration when assistance with publishing is needed. Authentication should be performed by the account owner in the provider's interface.
