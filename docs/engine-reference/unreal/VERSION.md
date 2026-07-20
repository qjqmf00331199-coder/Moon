# Unreal Engine — Version Reference

| Field | Value |
|-------|-------|
| **Engine Version** | Unreal Engine 5.8 |
| **Release Date** | ~June 2026 |
| **Project Pinned** | 2026-07-16 |
| **Last Docs Verified** | 2026-07-16 |
| **LLM Knowledge Cutoff** | May 2025 |
| **Risk Level** | HIGH — version is beyond LLM training data |

## Knowledge Gap Warning

The LLM's training data covers Unreal Engine up to ~5.3. Versions 5.4 through
5.8 introduced significant changes the model does NOT know about natively.
Always cross-reference this directory before suggesting Unreal API calls.
See `breaking-changes.md` and `deprecated-apis.md` for specifics.

## Post-Cutoff Version Timeline

| Version | Release | Risk Level | Key Theme |
|---------|---------|------------|-----------|
| 5.4 | ~Mid 2025 | HIGH | Motion Design tools, animation improvements, PCG enhancements |
| 5.5 | ~Sep 2025 | HIGH | Megalights (millions of lights), animation authoring, MegaCity demo |
| 5.6 | ~Oct 2025 | MEDIUM | Performance optimizations, bug fixes |
| 5.7 | Nov 2025 | HIGH | PCG production-ready, Substrate production-ready, AI assistant |
| 5.8 | ~Jun 2026 | HIGH | Megalights production-ready, Mass Framework overhaul, Iris production-ready, MCP Server (experimental), unified Input System |

## What Changed 5.7 → 5.8 (Summary)

See `breaking-changes.md` for full detail. Highlights:

- **Audio backend**: Windows default switched XAudio2 → WASAPI (opt-in fallback available, no code changes required)
- **Mass Framework**: reorganized into new `MassCore` module, separated from `MassGameplay` — may require import path updates for Mass-heavy projects
- **Input System**: Enhanced Input and Common Input unified — removes duplicate data asset requirements
- **Iris Replication**: now production-ready (was experimental)
- **Zen Server**: now default cooked output store; `AllowRemoteNetworkService` config key renamed to `RemoteNetworkService` with new enum values
- **Build toolchain**: community reports indicate 5.8 prefers MSVC v145 (VS 2026) — **unverified against official docs, flag if compile errors mention "not a preferred version"**
- **GAS tag granting**: `InheritableOwnedTagsContainer` direct property deprecated (`UE_DEPRECATED(5.3, ...)`) → use `UTargetTagsGameplayEffectComponent` (componentized GE authoring). See `breaking-changes.md` / `deprecated-apis.md`.

Epic's official release notes report **no breaking changes for typical gameplay C++ projects** — most 5.7 Marketplace plugins work unmodified (may need `.uplugin` EngineVersion bump).

## Verified Sources

- Official docs: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-release-notes
- What's new in 5.8: https://dev.epicgames.com/documentation/unreal-engine/whats-new
- UE 5.8 announcement: https://www.unrealengine.com/news/unreal-engine-5-8-is-now-available
- Migration guide (official): https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-migration-guide
- Community C++ migration notes (5.7→5.8, unverified toolchain claims): https://jakubpradeniak.com/posts/dev-notes/migrating-unreal-engine-5-8-cpp-project/
