The _v4 build completed stability validation without a source fix. The same build is the accumulated _v5 application.

**Verification.** All 15 CTests passed after enabling localhost access for the network tests. The suite covered all ten shape pairings, all six faces including floor and ceiling, sphere/wall CCD at 200 m/s, sleeping and wake-up, long-running invariants, CUDA parity, viewer startup and multiplayer. The first sandboxed run had three network failures; the full rerun with loopback access passed.

The 120-body demo ran on CPU and CUDA for 2,400 ticks per backend at seeds 1, 7, 42 and 2026. Every run had zero escaped ticks and zero NaNs, each initial population passed pairwise clearance and wall bounds checks, and the two backends matched final hashes for all four seeds. A 120-frame CUDA viewer capture completed. Matrix A still produced the recorded baseline hash 5327840572052581381 at seed 42 after three ticks.

**Performance.** At seed 42 the final run measured 0.945 ms median CPU and 1.032 ms median CUDA step time. For a same-length comparison over 1,200 ticks, the original demo measured 0.785 ms CPU and 0.883 ms CUDA, versus 0.905 ms CPU and 1.019 ms CUDA for the tuned demo in _v4. This measures backend time on the local machine, excluding rendering. Seed 42 CPU energy peaked at its first recorded tick (18,337 J) and fell to 721 J by tick 2,400.

**Limit.** The maximum pre-correction wall projection across the stress runs was 0.128 m. The existing wall correction kept bodies inside at the end of each checked tick. These checks cover the demo settings and the existing sphere/wall CCD cases. Arbitrary high-speed swept tests for other shape pairs were outside this validation.
