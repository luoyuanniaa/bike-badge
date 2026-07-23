# Bike Badge licensing

Bike Badge uses different licenses for software, hardware design sources, and
creative material. This document defines which license applies to each part of
the repository.

## Software and firmware — Apache-2.0

Unless a file says otherwise, all software and machine-readable configuration
is licensed under the Apache License 2.0. This includes:

- ESP32 firmware and UI code under `main/`;
- build files such as `CMakeLists.txt`, `sdkconfig.defaults`, and component
  manifests;
- future iPhone companion-app source code under `ios/` or a similar software
  directory; and
- code samples embedded in the documentation.

The complete license text is available in [`LICENSE`](LICENSE) and
[`LICENSES/Apache-2.0.txt`](LICENSES/Apache-2.0.txt). The SPDX identifier is
`Apache-2.0`.

## Hardware design sources — CERN-OHL-W-2.0

Original hardware design sources placed under `hardware/` are licensed under
the CERN Open Hardware Licence Version 2 — Weakly Reciprocal. This includes
schematics, PCB layouts, mechanical CAD, source drawings, bills of materials,
and manufacturing files, unless an individual file says otherwise.

The complete license text is available in
[`LICENSES/CERN-OHL-W-2.0.txt`](LICENSES/CERN-OHL-W-2.0.txt). The SPDX
identifier is `CERN-OHL-W-2.0`.

Prose documentation inside `hardware/` remains documentation and is licensed
under CC BY 4.0 unless it is inseparable from a covered hardware design source.

## Documentation and creative material — CC BY 4.0

Unless a file says otherwise, original prose documentation and original media
are licensed under Creative Commons Attribution 4.0 International. This
includes:

- `README.md`, `PROJECT_HANDOFF.md`, and files under `docs/`;
- original screenshots, illustrations, and non-brand UI artwork under
  `assets/`; and
- supporting explanatory documents under `hardware/`.

Code samples remain Apache-2.0 as described above. The complete CC BY 4.0 text
is available in [`LICENSES/CC-BY-4.0.txt`](LICENSES/CC-BY-4.0.txt). The SPDX
identifier is `CC-BY-4.0`.

## Trademarks and excluded material

The licenses above do not grant rights to project names, logos, badges, or
other brand identifiers. See [`TRADEMARKS.md`](TRADEMARKS.md).

Third-party components, fonts, images, reference designs, product names, and
trademarks remain subject to their own licenses and rights. A third-party
notice or license attached to a file overrides the directory-level default in
this document.

## Contributions

Unless explicitly agreed otherwise, a contribution is made under the license
that applies to the destination file or directory described above. A
contributor must have the right to submit the contribution under those terms.
