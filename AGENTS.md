# Repository Guidelines

## Project Structure & Module Organization
The solution entry point is `ZapretGUI.sln`, which loads the UI app inside `ZapretGUI/`. Core logic lives in `main.cpp`, configuration helpers in `data.hpp`, and transport integrations under `zapret/` and `singbox/`, with shared utilities housed in `tools/`. UI assets are split across `icons/`, `fonts/`, and the vendorized `imgui/` backend. Runtime services derive from `ServiceBase`: zapret stacks register in `vars::services`, Singbox helpers sit in `vars::singbox_services`, and the UI iterates `vars::render_services`; remember to touch all three when adding a feature. Prebuilt binaries and runtime bundles land in `ZapretGUI/x64/`, while third-party archives are tracked in `libs/`. Keep generated installers or logs out of source control; only commit reproducible assets in these folders.

## Build, Test, and Development Commands
- `msbuild ZapretGUI.sln /t:Build /p:Configuration=Release /p:Platform=x64` – release build used for packaging and distribution.
- `msbuild ZapretGUI.sln /t:Build /p:Configuration=Debug /p:Platform=x64` – debug build with console diagnostics enabled.
- `devenv ZapretGUI.sln /Build "Release|x64"` – invoke Visual Studio’s integrated build when you need the IDE debugger.
- `./ZapretGUI/x64/Release/MisterFish.exe -console` – run the GUI locally and surface runtime logging.

## Coding Style & Naming Conventions
Follow the Visual Studio defaults already in the tree: four-space indentation, braces on new lines, and grouped includes. Free functions prefer lower or camel case (`relaunch`, `sendStop`), namespaces segment subsystems (`vars`, `tools`, `zapret`), and resource identifiers remain uppercase. Keep headers self-contained, rely on `std::` facilities over platform macros where possible, and run the IDE formatter before committing.

## Testing Guidelines
There is no automated harness yet; rely on manual verification. After every change build the debug configuration, start `MisterFish.exe -console`, and exercise DPI bypass flows (zapret toggle, sing-box start/stop, notifications). When modifying network installers, validate silent flags (`-silent`, `-verysilent`) and confirm assets appear in `ZapretGUI/x64/` without stale caches. Document any known issues in the PR body.

## Commit & Pull Request Guidelines
Git history favours tight, imperative subjects (e.g., `remove useless hostlists exclude`). Keep messages under 72 characters, expanding with bullet points in the body when rationale is non-trivial. Each PR should link related issues, describe user-facing impact, list manual test steps, and include screenshots when UI changes are visible. Mark dependencies or migration steps explicitly so release notes stay accurate.

## Security & Configuration Tips
The app requires elevated privileges to patch network routes; never commit personal credentials, API keys, or user-specific zip payloads. Store experimental proxies under `.gitignore`d paths, and scrub `data.hpp` before pushing. When bringing in new zapret releases, verify checksums and update bundled archives inside `libs/` instead of hot-patching binaries in `x64/`.
