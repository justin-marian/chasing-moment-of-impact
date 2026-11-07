<table width="100%">
  <tr>
    <td align="left" valign="middle" width="45%">
      <h1>Chasing the Moment of<br>Impact</h1>
    </td>
    <td align="right" valign="middle" width="55%">
      <a href="https://isocpp.org/"><img src="https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&amp;logoColor=white" alt="C++20"></a>
      <a href="https://developer.nvidia.com/cuda-toolkit"><img src="https://img.shields.io/badge/CUDA-12.4%2B-76B900?logo=nvidia&amp;logoColor=white" alt="CUDA 12.4 or newer"></a>
      <a href="https://www.opengl.org/"><img src="https://img.shields.io/badge/OpenGL-4.6-5586A4?logo=opengl&amp;logoColor=white" alt="OpenGL 4.6"></a>
      <a href="https://cmake.org/"><img src="https://img.shields.io/badge/CMake-3.27%2B-064F8C?logo=cmake&amp;logoColor=white" alt="CMake 3.27 or newer"></a>
      <a href="https://ninja-build.org/"><img src="https://img.shields.io/badge/Ninja-build-000000" alt="Ninja"></a>
      <a href="https://www.python.org/"><img src="https://img.shields.io/badge/Python-3-3776AB?logo=python&amp;logoColor=white" alt="Python 3"></a>
      <a href="https://www.glfw.org/"><img src="https://img.shields.io/badge/GLFW-windowing-2B2B2B" alt="GLFW"></a>
      <a href="https://github.com/g-truc/glm"><img src="https://img.shields.io/badge/GLM-math-4B5563" alt="GLM"></a>
      <a href="https://github.com/Dav1dde/glad"><img src="https://img.shields.io/badge/GLAD-loader-4B5563" alt="GLAD"></a>
      <a href="https://github.com/ocornut/imgui"><img src="https://img.shields.io/badge/Dear%20ImGui-diagnostics-4B5563" alt="Dear ImGui"></a>
      <a href="https://github.com/nlohmann/json"><img src="https://img.shields.io/badge/nlohmann%2Fjson-run%20records-4B5563" alt="nlohmann/json"></a>
      <a href="https://www.microsoft.com/windows"><img src="https://img.shields.io/badge/Windows-x64-0078D4?logo=windows&amp;logoColor=white" alt="Windows x64"></a>
      <a href="https://www.kernel.org/"><img src="https://img.shields.io/badge/Linux-supported-FCC624?logo=linux&amp;logoColor=black" alt="Linux"></a>
    </td>
  </tr>
  <tr>
    <td colspan="2" align="left">
      <p><b>Physics of Rigid Bodies from Collision Prediction to Efficient Resolution through Iterative Constraints</b></p>
      <p><em>A 3D rigid-body engine that runs the same collision scenes on a CPU reference and on CUDA. Each tick is hashed, so you can check that both backends agree exactly, from the first contact to the last. The OpenGL viewer shows where objects hit and how hard.</em></p>
      <p>
      <img src="https://img.shields.io/badge/Deterministic-4B5563?style=flat-square&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI%2BPHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTkgMTYuMiA0LjggMTJsLTEuNCAxLjRMOSAxOSAyMSA3bC0xLjQtMS40eiIvPjwvc3ZnPg%3D%3D" alt="Deterministic">
      <img src="https://img.shields.io/badge/Parallel-4B5563?style=flat-square&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI%2BPHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTMgM2g0djE4SDN6TTEwIDNoNHYxOGgtNHpNMTcgM2g0djE4aC00eiIvPjwvc3ZnPg%3D%3D" alt="Parallel">
      <img src="https://img.shields.io/badge/Inspectable-4B5563?style=flat-square&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI%2BPHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTE1LjUgMTRoLS43OWwtLjI4LS4yN0E2LjQ3IDYuNDcgMCAwIDAgMTYgOS41IDYuNSA2LjUgMCAxIDAgOS41IDE2YzEuNjEgMCAzLjA5LS41OSA0LjIzLTEuNTdsLjI3LjI4di43OWw1IDQuOTlMMjAuNDkgMTl6bS02IDBDNy4wMSAxNCA1IDExLjk5IDUgOS41UzcuMDEgNSA5LjUgNSAxNCA3LjAxIDE0IDkuNSAxMS45OSAxNCA5LjUgMTQiLz48L3N2Zz4%3D" alt="Inspectable">
      </p>
      </td>
  </tr>
  <tr>
    <td colspan="2" align="center">
      <a id="simulation-gallery"></a>
      <p>
      <a href="media/matrices/cpu-A.gif"><img src="media/matrices/cpu-A.gif" height="120" alt="Matrix A on CPU, 850 bodies"></a>
      <a href="media/matrices/cpu-B.gif"><img src="media/matrices/cpu-B.gif" height="120" alt="Matrix B on CPU, 1,750 bodies"></a>
      <a href="media/matrices/cpu-C.gif"><img src="media/matrices/cpu-C.gif" height="120" alt="Matrix C on CPU, 4,000 bodies"></a>
      <a href="media/cpu-demo.gif"><img src="media/cpu-demo.gif" height="120" alt="Mixed shapes on CPU, 120 bodies"></a>
      <a href="media/cpu-impact.gif"><img src="media/cpu-impact.gif" height="120" alt="Impact strength on CPU"></a>
      </p>
      <p>
      <a href="media/matrices/cuda-A.gif"><img src="media/matrices/cuda-A.gif" height="120" alt="Matrix A on CUDA, 850 bodies"></a>
      <a href="media/matrices/cuda-B.gif"><img src="media/matrices/cuda-B.gif" height="120" alt="Matrix B on CUDA, 1,750 bodies"></a>
      <a href="media/matrices/cuda-C.gif"><img src="media/matrices/cuda-C.gif" height="120" alt="Matrix C on CUDA, 4,000 bodies"></a>
      <a href="media/cuda-demo.gif"><img src="media/cuda-demo.gif" height="120" alt="Mixed shapes on CUDA, 120 bodies"></a>
      <a href="media/cuda-impact.gif"><img src="media/cuda-impact.gif" height="120" alt="Impact strength on CUDA"></a>
      </p>
    </td>
  </tr>
</table>

<p align="center">
  <a href="#quick-start"><img src="https://img.shields.io/badge/Quick%20start-2563EB?style=flat-square&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI%2BPHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTggNXYxNGwxMS03eiIvPjwvc3ZnPg%3D%3D" height="28" alt="Quick start"></a>
  <a href="#highlights"><img src="https://img.shields.io/badge/Highlights-9333EA?style=flat-square&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI%2BPHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTEyIDE3LjI3IDE4LjE4IDIxbC0xLjY0LTcuMDNMMjIgOS4yNGwtNy4xOS0uNjFMMTIgMiA5LjE5IDguNjMgMiA5LjI0bDUuNDYgNC43M0w1LjgyIDIxeiIvPjwvc3ZnPg%3D%3D" height="28" alt="Highlights"></a>
  <a href="#results"><img src="https://img.shields.io/badge/Results-059669?style=flat-square&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI%2BPHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTUgOS4yaDNWMTlINXpNMTAuNiA1aDIuOHYxNGgtMi44em01LjYgOEgxOXY2aC0yLjh6Ii8%2BPC9zdmc%2B" height="28" alt="Results"></a>
  <a href="#under-the-hood"><img src="https://img.shields.io/badge/Under%20the%20hood-D97706?style=flat-square&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI%2BPHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTExLjk5IDE4LjU0bC03LjM3LTUuNzNMMyAxNC4wN2w5IDcgOS03LTEuNjMtMS4yN3pNMTIgMTZsNy4zNi01LjczTDIxIDlsLTktNy05IDcgMS42MyAxLjI3eiIvPjwvc3ZnPg%3D%3D" height="28" alt="Under the hood"></a>
  <a href="#configuration"><img src="https://img.shields.io/badge/Configuration-475569?style=flat-square&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI%2BPHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTMgMTd2Mmg2di0ySDN6TTMgNXYyaDEwVjVIM3ptMTAgMTZ2LTJoOHYtMmgtOHYtMmgtMnY2aDJ6TTcgOXYySDN2Mmg0djJoMlY5SDd6bTE0IDR2LTJIMTF2MmgxMHptLTYtNGgyVjdoNFY1aC00VjNoLTJ2NnoiLz48L3N2Zz4%3D" height="28" alt="Configuration"></a>
  <a href="#viewer"><img src="https://img.shields.io/badge/Viewer-0891B2?style=flat-square&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI%2BPHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTIwIDNINGMtMS4xIDAtMiAuOS0yIDJ2MTJjMCAxLjEuOSAyIDIgMmg3djJIOHYyaDh2LTJoLTN2LTJoN2MxLjEgMCAyLS45IDItMlY1YzAtMS4xLS45LTItMi0yem0wIDE0SDRWNWgxNnYxMnoiLz48L3N2Zz4%3D" height="28" alt="Viewer"></a>
  <a href="#outputs"><img src="https://img.shields.io/badge/Outputs-7C3AED?style=flat-square&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI%2BPHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTE0IDJINmMtMS4xIDAtMiAuOS0yIDJ2MTZjMCAxLjEuODkgMiAyIDJoMTJjMS4xIDAgMi0uOSAyLTJWOGwtNi02em0yIDE2SDh2LTJoOHYyem0wLTRIOHYtMmg4djJ6bS0zLTVWMy41TDE4LjUgOUgxM3oiLz48L3N2Zz4%3D" height="28" alt="Outputs"></a>
  <a href="#repository-layout"><img src="https://img.shields.io/badge/Layout-65A30D?style=flat-square&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI%2BPHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTEwIDRINGMtMS4xIDAtMS45OS45LTEuOTkgMkwyIDE4YzAgMS4xLjkgMiAyIDJoMTZjMS4xIDAgMi0uOSAyLTJWOGMwLTEuMS0uOS0yLTItMmgtOGwtMi0yeiIvPjwvc3ZnPg%3D%3D" height="28" alt="Layout"></a>
  <a href="#license"><img src="https://img.shields.io/badge/License-EAB308?style=flat-square&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI%2BPHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTIxIDVjLTEuMTEtLjM1LTIuMzMtLjUtMy41LS41LTEuOTUgMC00LjA1LjQtNS41IDEuNS0xLjQ1LTEuMS0zLjU1LTEuNS01LjUtMS41UzIuNDUgNC45IDEgNnYxNC42NWMwIC4yNS4yNS41LjUuNS4xIDAgLjE1LS4wNS4yNS0uMDVDMy4xIDIwLjQ1IDUuMDUgMjAgNi41IDIwYzEuOTUgMCA0LjA1LjQgNS41IDEuNSAxLjM1LS44NSAzLjgtMS41IDUuNS0xLjUgMS42NSAwIDMuMzUuMyA0Ljc1IDEuMDUuMS4wNS4xNS4wNS4yNS4wNS4yNSAwIC41LS4yNS41LS41VjZjLS42LS40NS0xLjI1LS43NS0yLTF6Ii8%2BPC9zdmc%2B" height="28" alt="License"></a>
</p>

---

<br>
<a id="quick-start" href="#quick-start"><img src="https://img.shields.io/badge/Quick%20start-2563EB?style=for-the-badge&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI%2BPHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTggNXYxNGwxMS03eiIvPjwvc3ZnPg%3D%3D" height="40" alt="Quick start"></a>
<br><br>


From the repository root, build the CPU and CUDA backends with the OpenGL viewer and launch Matrix A. The examples use SM 89. Set the architecture flag to your GPU’s compute capability.

**Windows · PowerShell**

```powershell
.\scripts\deployment\build-windows.ps1 -Cuda -Viewer -Architecture 89
.\exports\builds\windows-x64\physics_viewer.exe --backend cuda --scenario A
```

**Linux · Bash**

```bash
bash ./scripts/deployment/build-linux.sh --cuda --viewer --arch 89
./exports/builds/linux-$(uname -m)/physics_viewer --backend cuda --scenario A
```

For CPU-only use, omit `-Cuda` or `--cuda` and launch with `--backend cpu`. The CPU build supports the same box scenes and viewer controls. See [build requirements](#configuration) for toolchain and driver details.

<br>
<a id="highlights" href="#highlights"><img src="https://img.shields.io/badge/Highlights-9333EA?style=for-the-badge&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI%2BPHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTEyIDE3LjI3IDE4LjE4IDIxbC0xLjY0LTcuMDNMMjIgOS4yNGwtNy4xOS0uNjFMMTIgMiA5LjE5IDguNjMgMiA5LjI0bDUuNDYgNC43M0w1LjgyIDIxeiIvPjwvc3ZnPg%3D%3D" height="40" alt="Highlights"></a>
<br><br>


- **Reproducible.** CPU and CUDA matched all 720 per-tick state hashes in each recorded A/B/C verification run.
- **Parallel.** The CUDA solver groups contacts that affect different bodies so it can resolve them concurrently.
- **Preallocated.** The step loop reuses bounded working buffers after setup.
- **Inspectable.** Local heatmaps show where impacts occur and how much energy they carry.
- **Tested.** The suite includes a 1,000,000-tick stack stability check.
- **Measurable.** The command-line runner exports per-stage timings, contact counts, and state hashes.

<br>
<a id="results" href="#results"><img src="https://img.shields.io/badge/Results-059669?style=for-the-badge&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI%2BPHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTUgOS4yaDNWMTlINXpNMTAuNiA1aDIuOHYxNGgtMi44em01LjYgOEgxOXY2aC0yLjh6Ii8%2BPC9zdmc%2B" height="40" alt="Results"></a>
<br>

### Correctness

The November 7, 2026 verification advanced each matrix for 720 ticks on both backends. A matching tick means the independently computed state hashes were equal. An escaped tick is a tick where the containment diagnostics found a body outside the enclosure.

| Scenario | Bodies | Matching ticks | Escaped ticks (CPU / CUDA) | Final state hash (both) |
| --- | ---: | ---: | ---: | --- |
| [Matrix A](configs/scenarios_matrix_a.yaml) | 850 | 720/720 | 0 / 0 | `3330260331900169050` |
| [Matrix B](configs/scenarios_matrix_b.yaml) | 1,750 | 720/720 | 0 / 0 | `3349342891293941449` |
| [Matrix C](configs/scenarios_matrix_c.yaml) | 4,000 | 720/720 | 0 / 0 | `11731164910491628103` |

| Verification setting | Recorded value |
| --- | --- |
| Initial conditions | Seed 42 · 720 ticks · fixed step of 1/120 s · float32 state |
| Hardware | Intel Core i9-14900HX · NVIDIA GeForce RTX 4070 Laptop GPU |
| Toolchain | Windows x64 · MSVC 19.51 · CUDA Toolkit 13.4.92 · SM 89 |

These results describe the recorded runs. The longer stack stability check is a separate test in the suite.

### Performance

| Scenario | CPU median / p95 (ms/tick) | CUDA median / p95 (ms/tick) | Contact samples (CPU / CUDA) |
| --- | --- | --- | --- |
| Matrix A | pending | pending | pending |
| Matrix B | pending | pending | pending |
| Matrix C | pending | pending | pending |

Use the same tick window on both backends and report the median and 95th percentile of `total_wall_ms` from `frames.csv`. The median describes a typical tick; p95 shows how long the slower ticks take. Sum `contacts_i` over the window for contact samples. A persistent contact can contribute once per tick, so that sum does not count unique collisions. Record the hardware, toolchain, and source fingerprint alongside measured timings.

See the [simulation gallery](#simulation-gallery) for side-by-side recordings of all three matrices.

### Reproduce the parity check

Record Matrix A on the CPU and replay its initial state and configuration on CUDA. Replay compares the state hash on each recorded tick. Use `B` or `C` in place of `A` to check the larger matrices.

```powershell
$physics = '.\exports\builds\windows-x64\physics.exe'
& $physics --backend cpu --scenario A --seed 42 --ticks 720 --output generated/reports/matrix-a-cpu
& $physics --backend cuda --replay generated/reports/matrix-a-cpu
```

```bash
physics="./exports/builds/linux-$(uname -m)/physics"
"$physics" --backend cpu --scenario A --seed 42 --ticks 720 --output generated/reports/matrix-a-cpu
"$physics" --backend cuda --replay generated/reports/matrix-a-cpu
```

The CTest suite checks position and velocity agreement across five seeds and verifies repeatable CUDA hashes on A/B/C. The replay command checks exact per-tick hashes for the specific recorded run.

<br>
<a id="under-the-hood" href="#under-the-hood"><img src="https://img.shields.io/badge/Under%20the%20hood-D97706?style=for-the-badge&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI%2BPHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTExLjk5IDE4LjU0bC03LjM3LTUuNzNMMyAxNC4wN2w5IDcgOS03LTEuNjMtMS4yN3pNMTIgMTZsNy4zNi01LjczTDIxIDlsLTktNy05IDcgMS42MyAxLjI3eiIvPjwvc3ZnPg%3D%3D" height="40" alt="Under the hood"></a>
<br>

### State and candidate pairs

The engine stores each body property in its own contiguous array. This **structure of arrays** lets a pass work with the properties it needs, such as positions and bounds. Impact telemetry allocates its buffers on first use and then reuses them.

| Responsibility | CPU reference | CUDA backend |
| --- | --- | --- |
| State | Advances host arrays on a single thread. | Advances device buffers and downloads state when requested. |
| Candidate ordering | Sorts grid entries by cell key and body ID. | Uses a stable radix sort that preserves body ID order within each cell. |
| Contact solving | Applies impulses in sequence. | Resolves contacts in parallel within each color group, then synchronizes before the next group. |

The **broad phase** searches nearby cells of a spatial grid and filters candidate pairs by their bounds. The **narrow phase** tests those pairs against the actual shapes and supplies contact points, normals, and penetration depths to the solver. Supported shapes are spheres, cubes, cylinders, and cones.

### Contact resolution

Each contact constrains motion along its normal and applies friction along the surface. The sequential impulse solver revisits those constraints for a fixed number of passes. An impulse changes the velocities read by later passes, so contacts that share a dynamic body must be processed in order.

**Graph coloring** assigns contacts that share no dynamic body to the same group. CUDA resolves the contacts within each group in parallel and synchronizes before moving to the next group. The current solve runs in one CUDA block, so scaling depends on the number and size of the color groups. After velocity solving, a separate position correction reduces remaining overlap. The wall path includes continuous collision detection for fast spheres, and the position pass checks box containment.

### Ten stages per fixed tick

| # | Operation | Work performed |
| ---: | --- | --- |
| 1 | Apply forces | Reset force accumulators and apply gravity. |
| 2 | Integrate velocity | Update linear and angular velocities using the fixed time step. |
| 3 | Refresh bounds | Recompute world-space axis-aligned bounding boxes (AABBs). |
| 4 | Broad phase | Gather candidate pairs from the sorted spatial grid. |
| 5 | Body contacts | Test candidate shapes and build body-to-body contacts. |
| 6 | World contacts | Build contacts against the six enclosure faces. |
| 7 | Solve contacts | Apply normal and friction impulses, then correct residual overlap. |
| 8 | Integrate pose | Advance positions and orientations and enforce wall containment. |
| 9 | Update sleep | Let supported, quiet bodies rest and wake them when contacts disturb them. |
| 10 | Emit diagnostics | Collect timing, energy, momentum, contact, and containment counters. |

### Numerical reproducibility

Both backends use the same fixed-step configuration and shared collision math. CPU compilation uses strict floating-point settings, and CUDA disables fused multiply-add contraction with `--fmad=false`. When comparing hashes, use the same initial state, scenario, precision, and tick count. Record the compiler and device alongside the results so a later run has the context needed to reproduce the comparison.

<br>
<a id="configuration" href="#configuration"><img src="https://img.shields.io/badge/Configuration-475569?style=for-the-badge&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI%2BPHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTMgMTd2Mmg2di0ySDN6TTMgNXYyaDEwVjVIM3ptMTAgMTZ2LTJoOHYtMmgtOHYtMmgtMnY2aDJ6TTcgOXYySDN2Mmg0djJoMlY5SDd6bTE0IDR2LTJIMTF2MmgxMHptLTYtNGgyVjdoNFY1aC00VjNoLTJ2NnoiLz48L3N2Zz4%3D" height="40" alt="Configuration"></a>
<br>

### Build requirements

| Build profile | Requirements |
| --- | --- |
| Base CPU build | CMake 3.27+, Ninja, Git, and a C++20 compiler. The Windows script uses MSVC and the Windows SDK. |
| CUDA | Base tools, CUDA Toolkit 12.4+, a supported host compiler, and a CUDA-capable GPU. |
| OpenGL viewer | Base tools, an OpenGL 4.6 driver, and Python 3 for GLAD generation. On Linux, install the development headers for the window system used by GLFW. |

CMake fetches pinned dependencies into the build directory during configuration. The first configure needs network access.

| Windows option | Linux option | Purpose |
| --- | --- | --- |
| `-Cuda` | `--cuda` | Build CUDA alongside the CPU backend. |
| `-Viewer` | `--viewer` | Build the interactive OpenGL viewer. |
| `-Architecture 89` | `--arch 89` | Select the CUDA target architecture. |
| `-ReferenceDouble` | `--reference-double` | Build the CPU-only float64 reference. |

### Scene settings

The headless runner accepts `--backend cpu` or `--backend cuda`, `--scenario A`, `B`, `C`, or `demo`, and `--config FILE` for a flat YAML configuration. The three matrix files in [configs/](configs/) are starting points for custom scenes.

| YAML fields | What they control |
| --- | --- |
| `seed`, `spheres_i`, `cubes_i`, `cylinders_i`, `cones_i` | Initial random seed and body counts by shape. |
| `radius_m`, `boxHalfExtentX_m`, `boxHalfExtentY_m`, `boxHalfExtentZ_m` | Initial body size and enclosure half extents in metres. |
| `dt_s`, `gravity_mps2`, `maxInitialSpeed_mps` | Fixed step, gravity, and the cap on initial speed. |
| `restitution_unitless`, `frictionStatic_unitless`, `frictionDynamic_unitless` | Bounce and friction response. |
| `cellSize_m`, `solverIterations_i`, `enableSleep`, `enableCcd` | Broad-phase cell size, solver passes, sleeping, and continuous collision detection. |

Use `--seed N` after the scenario or config argument to override its seed. Use `--ticks N` for run length and `--output DIR` to give each run its own log directory.

### Run the tests

```powershell
ctest --test-dir tmp/build/windows --output-on-failure
ctest --test-dir tmp/build/windows --output-on-failure -R '^cuda_parity_and_determinism$'
```

```bash
ctest --test-dir tmp/build/linux --output-on-failure
ctest --test-dir tmp/build/linux --output-on-failure -R '^cuda_parity_and_determinism$'
```

The CUDA parity test is registered by CUDA-enabled builds.

<br>
<a id="viewer" href="#viewer"><img src="https://img.shields.io/badge/Viewer-0891B2?style=for-the-badge&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI%2BPHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTIwIDNINGMtMS4xIDAtMiAuOS0yIDJ2MTJjMCAxLjEuOSAyIDIgMmg3djJIOHYyaDh2LTJoLTN2LTJoN2MxLjEgMCAyLS45IDItMlY1YzAtMS4xLS45LTItMi0yem0wIDE0SDRWNWgxNnYxMnoiLz48L3N2Zz4%3D" height="40" alt="Viewer"></a>
<br><br>

Launch with `--scenario demo` or `--scenario impact` to explore the two [collision demonstrations](#simulation-gallery).

### Controls

| Action | Control |
| --- | --- |
| Start, pause, or reset | **Run**, **Pause**, or **Reset** in the toolbar. |
| Advance exactly one tick | Pause, open **Physics settings** from **Widgets**, then select **Step once**. |
| Reframe the box | Adjust **Orbit**, **Elevation**, and **Zoom** in **Camera & impact display**. |
| Inspect collision bounds | Toggle **AABB**, **OBB**, or **Velocity**. An OBB is a bounding box that rotates with the object. |
| Inspect impact severity | Toggle **Impact heat coloring**, then adjust the fade time or red-energy threshold. |
| Switch backends | Select **CPU** or **CUDA** in **Simulation**. |
| Hide the overlay | Press **F1**. Use **Widgets** to show individual panels. |

For a repeatable still, use `--hidden --presentation --frames N --capture FILE.png`. The viewer saves the final frame and a JSON sidecar with its simulation state hash and capture settings.

<br>
<a id="outputs" href="#outputs"><img src="https://img.shields.io/badge/Outputs-7C3AED?style=for-the-badge&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI%2BPHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTE0IDJINmMtMS4xIDAtMiAuOS0yIDJ2MTZjMCAxLjEuODkgMiAyIDJoMTJjMS4xIDAgMi0uOSAyLTJWOGwtNi02em0yIDE2SDh2LTJoOHYyem0wLTRIOHYtMmg4djJ6bS0zLTVWMy41TDE4LjUgOUgxM3oiLz48L3N2Zz4%3D" height="40" alt="Outputs"></a>
<br><br>

The headless runner creates these files under the directory passed to `--output`.

| File | Use |
| --- | --- |
| `frames.csv` | Per-tick stage timings, backend time, contact counts, candidate pairs, and physical diagnostics. |
| `trace.csv` | Tick number and state hash, used to locate the first divergence during replay. |
| `manifest.json` | Backend, hardware, compiler, seed, precision, source fingerprint, and final state hash. |
| `config.json`, `initial.json` | Configuration and exact starting state read by replay. |
| `final.json`, `motion.csv` | Final body state and motion data for inspection. |

<details>
<summary><strong>Recorded Matrix A output · selected fields</strong></summary>

CPU run, seed 42. The CSV excerpt shows the first four ticks, with elapsed simulation time rounded to six decimal places. The manifest excerpt describes the complete 720-tick run.

```csv
tick_i,t_s,contacts_i,candidatePairs_i
1,0.008333,24,9
2,0.016667,16,6
3,0.025000,32,6
4,0.033333,41,6
```

```json
{
  "bodyCount_i": 850,
  "seed": 42,
  "ticks_i": 720,
  "realBytes_i": 4,
  "replayHash": "3330260331900169050"
}
```

</details>

Windows builds place the executables and app-local DLLs under `exports/builds/windows-x64/`. In a checkout that includes prebuilt binaries, run `git lfs pull` to fetch them. Keep the DLLs beside the executables and keep the repository’s shaders and fonts available to the viewer. [Git LFS rules](.gitattributes)

<br>
<a id="repository-layout" href="#repository-layout"><img src="https://img.shields.io/badge/Repository%20layout-65A30D?style=for-the-badge&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI%2BPHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTEwIDRINGMtMS4xIDAtMS45OS45LTEuOTkgMkwyIDE4YzAgMS4xLjkgMiAyIDJoMTZjMS4xIDAgMi0uOSAyLTJWOGMwLTEuMS0uOS0yLTItMmgtOGwtMi0yeiIvPjwvc3ZnPg%3D%3D" height="40" alt="Repository layout"></a>
<br><br>

| Area | Path | Responsibility |
| --- | --- | --- |
| Simulation data | [src/shared/](src/shared/) | Body storage, geometric primitives, and common simulation math. |
| Physics | [src/physics/](src/physics/) | CPU and CUDA backends, scene generation, replay, and logging. |
| Graphics | [src/client/](src/client/) | OpenGL renderer and Dear ImGui controls. |
| Entry points | [src/demos/](src/demos/) | Command-line runner and viewer. |
| Defaults | [src/settings/](src/settings/) | Compute, collision, and rendering constants. |
| Build | [infra/](infra/) · [scripts/deployment/](scripts/deployment/) | CMake targets and dependencies, plus Windows and Linux build scripts. |
| Scenes | [configs/](configs/) | YAML definitions for Matrix A, B, and C. |
| Tests | [tests/unit/](tests/unit/) | Collision, parity, impact, viewer, and stability checks. |
| Media | [media/](media/) | Simulation recordings and PNG stills. |

<br>
<a id="acknowledgements" href="#acknowledgements"><img src="https://img.shields.io/badge/Acknowledgments-0F766E?style=for-the-badge&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI%2BPHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTEyIDIxLjM1bC0xLjQ1LTEuMzJDNS40IDE1LjM2IDIgMTIuMjggMiA4LjUgMiA1LjQyIDQuNDIgMyA3LjUgM2MxLjc0IDAgMy40MS44MSA0LjUgMi4wOUMxMy4wOSAzLjgxIDE0Ljc2IDMgMTYuNSAzIDE5LjU4IDMgMjIgNS40MiAyMiA4LjVjMCAzLjc4LTMuNCA2Ljg2LTguNTUgMTEuNTRMMTIgMjEuMzV6Ii8%2BPC9zdmc%2B" height="40" alt="Acknowledgments"></a>
<br><br>

The engine uses nlohmann/json for run records, GLFW for windowing, GLAD for OpenGL loading, GLM for renderer math, and Dear ImGui for the viewer controls. [nlohmann/json](https://github.com/nlohmann/json) · [GLFW](https://www.glfw.org/) · [GLAD](https://github.com/Dav1dde/glad) · [GLM](https://github.com/g-truc/glm) · [Dear ImGui](https://github.com/ocornut/imgui)

<br>
<a id="license" href="#license"><img src="https://img.shields.io/badge/License-EAB308?style=for-the-badge&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI%2BPHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTIxIDVjLTEuMTEtLjM1LTIuMzMtLjUtMy41LS41LTEuOTUgMC00LjA1LjQtNS41IDEuNS0xLjQ1LTEuMS0zLjU1LTEuNS01LjUtMS41UzIuNDUgNC45IDEgNnYxNC42NWMwIC4yNS4yNS41LjUuNS4xIDAgLjE1LS4wNS4yNS0uMDVDMy4xIDIwLjQ1IDUuMDUgMjAgNi41IDIwYzEuOTUgMCA0LjA1LjQgNS41IDEuNSAxLjM1LS44NSAzLjgtMS41IDUuNS0xLjUgMS42NSAwIDMuMzUuMyA0Ljc1IDEuMDUuMS4wNS4xNS4wNS4yNS4wNS4yNSAwIC41LS4yNS41LS41VjZjLS42LS40NS0xLjI1LS43NS0yLTF6Ii8%2BPC9zdmc%2B" height="40" alt="License"></a>
<br><br>

The project is released under the MIT License. Vulnerability reporting follows the repository security policy. [LICENSE](LICENSE) · [SECURITY.md](SECURITY.md)
