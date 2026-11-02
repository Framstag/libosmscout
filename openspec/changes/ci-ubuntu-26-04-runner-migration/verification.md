# Verification

Filled in from the pull request run; see the task list in `tasks.md`.

## Local pre-checks

- `openspec validate ci-ubuntu-26-04-runner-migration --strict` — result recorded below.
- Every package the Linux jobs install was checked against the `resolute` (26.04) archive:

  ```
  catch2 → transitional, points at libcatch2-dev (3.7.1)
  libcatch2-dev 3.7.1-0.6, clang-21 1:21.1.8-6ubuntu1, libclang-21-dev, libc++-dev,
  libagg-dev, libmarisa-dev, libtbb-dev, libglut-dev, libglew-dev, libglfw3-dev,
  libxml2-dev, nlohmann-json3-dev, libprotobuf-dev, protobuf-compiler, liblzma-dev,
  libpng-dev, libfreetype6-dev, libcairo2-dev, libpangocairo-1.0-0, libpango1.0-dev,
  qtdeclarative5-dev, libqt5svg5-dev, qtlocation5-dev, qtpositioning5-dev,
  qttools5-dev, qttools5-dev-tools, qtmultimedia5-dev, qmake6, libqt6svg6-dev,
  libqt6core5compat6-dev, qt6-declarative-dev, qt6-positioning-dev, qt6-tools-dev,
  qt6-tools-dev-tools, qt6-l10n-tools, qt6-multimedia-dev, libglm-dev, fonts-liberation,
  xvfb — all present.
  ```

## CI

Run of the pull request on `ubuntu-26.04`; to be recorded.
