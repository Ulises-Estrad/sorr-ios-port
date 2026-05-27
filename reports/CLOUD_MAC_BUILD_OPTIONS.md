# Cloud Mac Build Options

Date: 2026-05-27

## Goal

Pick the lowest-friction way to prove the iOS shell on macOS/Xcode, then later build the x64-safe runtime path for iOS.

Current local limitation:

- Windows can build and prove desktop x64.
- Windows cannot run Xcode, iOS SDKs, iOS Simulator, or device signing.

## Recommendation

Fastest path for shell proof:

1. Borrowed Mac, if available for even one session.
2. GitHub Actions macOS runner, if the project can be pushed to a private repo and simulator logs/artifacts are enough.
3. Codemagic or Bitrise, if mobile-CI convenience is worth the setup.
4. MacStadium, if interactive remote Mac access will be useful across multiple sessions.
5. AWS EC2 Mac only if you already have AWS setup and accept the 24-hour Mac host friction.

For the current milestone, a simulator `.app` build is enough. Device install/signing can wait.

## Option Comparison

| Option | CMake/Xcode | SDL2 iOS setup | Logs/artifacts | Signing/device install | Cost/friction | Fit for next step |
| --- | --- | --- | --- | --- | --- | --- |
| GitHub Actions macOS | Good for scripted `cmake`/`xcodebuild` | Build SDL2 from source or cache a framework | Strong artifact upload and logs | Possible with secrets/certs, but simulator proof needs no signing | Low setup if repo can be pushed; paid minutes may matter | Best first cloud proof |
| Codemagic | Strong mobile CI | Good for scripted setup, mobile-oriented docs | Good build history/artifacts | Strong signing/distribution workflow | More account setup, but mobile-first | Good if GitHub Actions gets annoying |
| Bitrise | Strong mobile CI | Good, with Xcode simulator workflows | Good artifact tabs/deploy steps | Strong signing support; simulator builds do not need signing | More UI/workflow setup | Good for simulator `.app` and later signed IPA |
| MacStadium | Full remote Mac control | Manual but flexible | You control logs/artifacts | Full signing/device paths if configured | Higher cost/setup than CI | Best persistent remote Mac |
| AWS EC2 Mac | Full remote Mac control | Manual but flexible | You control logs/artifacts | Full signing/device paths if configured | Highest operational friction; 24-hour host minimum | Useful if AWS infra already exists |
| Borrowed Mac | Full local control | Manual, easiest to debug interactively | Manual capture | Easiest real device path if Apple ID/certs available | Lowest cloud cost, availability-dependent | Best debugging experience |

## GitHub Actions macOS

Why it fits:

- Runs macOS hosted runners with Xcode-capable images.
- Easy to run `cmake`, `xcodebuild`, `xcrun`, and shell scripts.
- Easy artifact capture with standard workflow artifact upload.
- No device signing needed for simulator shell proof.

Useful approach:

```yaml
runs-on: macos-15
steps:
  - uses: actions/checkout@v4
  - run: xcodebuild -version
  - run: cmake --version
  - run: ./ci/build-sdl2-ios.sh
  - run: ./ci/configure-ios-shell.sh
  - run: ./ci/build-ios-shell-simulator.sh
  - uses: actions/upload-artifact@v4
    with:
      name: sorr-ios-shell-sim
      path: build-ios-shell-sim
```

Pros:

- Best balance of speed, scriptability, and artifact capture.
- Works well for unsigned simulator `.app` proof.
- Good for repeatable CI once the project is in a repo.

Cons:

- Less interactive than a real Mac.
- Simulator launch can be finicky in headless CI.
- macOS runner minutes cost more than Linux/Windows minutes.
- Exact Xcode versions follow hosted image policy.

Sources:

- GitHub hosted runner reference: https://docs.github.com/en/actions/reference/runners/github-hosted-runners
- GitHub runner images and Xcode image policy: https://github.com/actions/runner-images

## Codemagic

Why it fits:

- Mobile-focused CI/CD with iOS support.
- Good artifact storage and build history.
- Strong signing/distribution support later.
- Can be driven by `codemagic.yaml`.

Pros:

- Less raw CI plumbing than GitHub Actions.
- Good later path for signed IPA builds.
- Mobile CI docs are tuned for iOS workflows.

Cons:

- Another CI platform to configure.
- Less direct control than a rented Mac.
- May be overkill for the first shell proof.

Best use here:

- Build SDL2 for iOS in a script step.
- Configure CMake/Xcode shell target.
- Save simulator `.app`, `xcodebuild` logs, and CMake logs as artifacts.

Sources:

- Codemagic docs: https://docs.codemagic.io/
- Codemagic pricing/artifact retention docs: https://docs.codemagic.io/billing/pricing/
- Codemagic iOS signing docs: https://docs.codemagic.io/yaml-code-signing/signing-ios/

## Bitrise

Why it fits:

- Built specifically around mobile workflows.
- Has simulator build/deploy concepts.
- Simulator builds can avoid code signing.
- Good artifact and workflow UI.

Pros:

- Friendly for iOS app workflows.
- Strong later route for signed builds.
- Simulator `.app` proof is a natural workflow.

Cons:

- Workflow/UI setup overhead.
- Less direct than a real Mac for low-level SDL/CMake debugging.
- Cost model and artifact retention need account-specific checking.

Best use here:

- Use script steps for SDL2+CMake if the standard Xcode steps do not fit.
- Capture simulator `.app`, CMake logs, and `xcodebuild` logs.

Sources:

- Bitrise simulator build docs: https://devcenter.bitrise.io/en/deploying/ios-deployment/deploying-an-ios-app-for-simulators.html
- Bitrise artifact docs: https://devcenter.bitrise.io/en/builds/managing-build-files.html
- Bitrise pricing: https://bitrise.io/pricing

## MacStadium

Why it fits:

- Real remote Mac access with Xcode.
- Better for interactive debugging than headless CI.
- Can become a persistent build machine or self-hosted runner.

Pros:

- Full control over Xcode, SDL2 builds, CMake, simulator, and logs.
- Easier to debug SDL/iOS lifecycle issues than CI.
- Good if the project needs multiple Mac sessions.

Cons:

- Higher setup and cost than quick CI.
- Requires remote access management.
- More manual maintenance.

Best use here:

- Rent a short-lived Mac, build SDL2/iOS shell manually, then automate the commands once proven.

Source:

- MacStadium pricing/options: https://macstadium.com/pricing

## AWS EC2 Mac

Why it fits:

- Real Mac hardware in AWS.
- Scriptable through SSH and AWS tooling.
- Full control if you already live in AWS.

Pros:

- Can run CMake/Xcode/SDL2 exactly like a remote Mac.
- Good artifact/log capture through S3 or normal CI tooling.
- Can support signing if certificates/secrets are installed.

Cons:

- Highest friction for this project.
- Requires Dedicated Host allocation.
- AWS documents a 24-hour minimum allocation period for Mac Dedicated Hosts.
- More cloud infrastructure work than the shell proof needs.

Best use here:

- Only use if GitHub Actions/Codemagic/Bitrise are blocked or if persistent AWS-hosted Mac automation is already desired.

Sources:

- AWS EC2 Mac docs: https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/ec2-mac-instances.html
- AWS EC2 Mac instance overview: https://aws.amazon.com/ec2/instance-types/mac/

## Borrowed Mac

Why it fits:

- Lowest conceptual friction.
- Best for the first human-observed simulator shell launch.
- Lets us see Xcode errors, simulator UI, SDL app lifecycle, and logs directly.

Pros:

- No CI platform setup.
- Easy interactive simulator debugging.
- Easiest path to real device install later if signing is available.

Cons:

- Depends on access to a Mac.
- Reproducibility must be captured manually.
- Build environment may be temporary.

Best use here:

- Run `reports/MACOS_XCODE_IOS_BUILD_STEPS.md` commands directly.
- Capture `xcodebuild -version`, `cmake --version`, simulator SDK path, build logs, and first shell logs.

## Practical First Cloud Milestone

For GitHub Actions or mobile CI, the first job should:

1. Print tool versions:
   - `xcodebuild -version`
   - `xcrun --sdk iphonesimulator --show-sdk-path`
   - `cmake --version`
2. Build or fetch SDL2 for iOS.
3. Configure `sorr-vita-master/cmake/ios`.
4. Build `SorrIOSShell` for `iphonesimulator`.
5. Upload:
   - CMake configure log,
   - `xcodebuild` log,
   - built `.app` or `.xcarchive` if produced,
   - any simulator run logs.

Do not bundle `SorR.dat` in this milestone.

## Current Pick

Recommended default: GitHub Actions macOS.

Reason:

- enough for shell proof,
- easiest to automate in repo form,
- good artifacts/logs,
- no signing needed for simulator,
- lower commitment than renting a persistent Mac.

Fallback if simulator launch is too headless or SDL/iOS lifecycle debugging needs UI: borrowed Mac or MacStadium.
