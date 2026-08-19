# Epic: Camera System Foundation Fixes

> **Layer**: Core
> **GDD**: `design/gdd/camera-system-base.md`
> **Architecture Module**: Camera System
> **Status**: Ready
> **Stories**: 9 stories created (see table below)

## Overview

Move the camera system from hardcoded constructor tuning toward the ADR-0005 data-driven SpringArm contract while preserving the already tuned shoulder camera feel. This epic owns camera settings data, pitch clamp ownership, lag reset behavior, FOV/execution blend hooks, collision/debris guardrails, and camera-shake limits.

## Governing ADRs

| ADR | Decision Summary | Engine Risk |
|---|---|---|
| ADR-0005: Camera System SpringArm | Keeps SpringArm/Camera, introduces `UMoonCameraSettings`, `AMoonPlayerCameraManager`, FOV/execution blend hooks, and camera-shake caps. | LOW |

## GDD Requirements

| TR-ID | Requirement | ADR Coverage |
|---|---|---|
| TR-cam-001 | SpringArm to Camera hierarchy on player character. | ADR-0005 ✅ |
| TR-cam-002 | Enhanced Input look routing and PlayerCameraManager pitch clamp. | ADR-0005 ✅ |
| TR-cam-003 | Camera-relative movement basis reads controller yaw independently. | ADR-0005 ⚠️ |
| TR-cam-004 | SpringArm lag with hard 60uu max distance. | ADR-0005 ⚠️ |
| TR-cam-005 | SpringArm collision and debris ignore policy. | ADR-0005 ✅ |
| TR-cam-006 | Overdrive FOV and execution presentation blends. | ADR-0005 ✅ |
| TR-cam-007 | `ResetCameraLag()` path on teleport/checkpoint. | ADR-0005 ✅ |
| TR-cam-008 | Look suppression for execution and camera-shake limits. | ADR-0005 ✅ |
| TR-cam-009 | Camera parameters are data-asset driven. | ADR-0005 ✅ |

## Stories

| # | Story | Type | Status | ADR |
|---|-------|------|--------|-----|
| 001 | Camera Hierarchy + Data-Driven Settings Foundation | Logic | Ready | ADR-0005 |
| 002 | Pitch Clamp via PlayerCameraManager | Logic | Ready | ADR-0005 |
| 003 | Camera-Relative Movement Basis + Facing Snap Verification | Logic | Ready | ADR-0005 |
| 004 | SpringArm Lag Max-Distance Hard-Follow | Logic | Ready | ADR-0005 |
| 005 | Collision Guardrails — Debris Ignore + Corner Dithering | Integration | Ready | ADR-0005 |
| 006 | Overdrive FOV Interpolation Hook | Logic | Ready | ADR-0005 |
| 007 | Execution Cutscene Camera Blend + Look-Input Suppression | Integration | Ready | ADR-0005 |
| 008 | ResetCameraLag on Teleport/Checkpoint | Integration | Ready | ADR-0005 |
| 009 | Camera Shake Dispatch + Amplitude Cap | Logic | Ready | ADR-0005 |

## Definition of Done

This epic is complete when camera QA-TEST-01 through QA-TEST-10 are executable in PIE, all runtime camera tuning comes from `UMoonCameraSettings`, and the strafe-aim facing regression remains covered by the Player Movement epic.

## Next Step

Run `/create-stories camera-system-foundation-fixes` after Player Movement story 001 is complete.
