# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Kitsas (formerly Kitupiikki) is a Finnish bookkeeping desktop application for small organisations, written in C++ with Qt (Widgets, Sql, Pdf, PdfWidgets, WebEngineWidgets, Network, Svg, Xml, PrintSupport; `core5compat` + `svgwidgets` on Qt 6). Qt ≥ 6.4 is required (6.8 for all features). Zip handling uses libzip.

**Everything is in Finnish** — class names, identifiers, comments, commit messages, UI strings. Follow that convention in new code and commits (e.g. "Korjattu …", "Lisätty …").

## Build

QMake-based. The top-level `kitsasproject.pro` is a `subdirs` project containing the app plus the test projects.

```bash
qmake kitsasproject.pro && make qmake_all
make
```

To build only the app: `qmake kitsas/kitsas.pro && make`.

- `kitsas/kitsas.pro` holds platform-specific library paths (libzip on Linux, openjpeg/bcrypt on Windows via hardcoded relative paths) — edit a local copy as needed.
- `kitsas/kitsas.pri` holds Qt module config; `kitsas/sources.pri` lists **every** source/header/ui/qrc file. New files must be added to `sources.pri` (shared by the app and test projects).
- `kitsas/versio.h` defines `KITSAS_VERSIO`, `KITSAS_API` (cloud API URL) and optional `KITSAS_DEVEL` / `KITSAS_PORTABLE` flags.
- Packaging/cross-compile scripts live in the repo root (`kaanna-*.sh` for MXE Windows builds, `paketoi-*.sh` for Linux packages, `mac/`).

## Tests

Qt Test (`QTest`) executables, each its own qmake project:

- `unittest/*` — small focused tests (`AlvLaskelmaTesti`, `KieliTesti`, `ValidatorTest`, `eurotest`, `tositerivitesti`, `viitetesti`). Each `.pro` either pulls in only the needed sources via `VPATH`/`INCLUDEPATH` to `../../kitsas`, or includes `unittest/apptest.pri` (which includes the full `sources.pri`).
- `testit/testit.pro` — integration-style suite with a custom runner (`testit/testit.cpp`) that registers test classes in a map. Run a single class with:
  ```bash
  ./testit -select <name> [QTest args]
  ```
  where `<name>` is a key registered in `testit.cpp` (`kpdateedit`, `tuonti`, `tili`, `tulomenorivi`, `tulomenoapuri`). Note: the selection only triggers when more than 3 arguments are given, so pass at least one extra QTest argument (e.g. `-v1`). New test classes must be added to the map in `testit.cpp` and to `testit.pro`.
- Standard QTest filtering works for single functions: `./tst_x <testFunction>`.

## Architecture

### Data access: REST-style API over two backends

The core design decision: all bookkeeping data is accessed through a REST-like interface, regardless of whether the book is a local SQLite file or hosted on Kitsas Oy's cloud server.

- `db/yhteysmodel.h` — abstract `YhteysModel` (connection). Implementations: `sqlite/SQLiteModel` (local file) and `pilvi/PilviModel` (cloud, `KITSAS_API`).
- `db/kpkysely.h` — abstract `KpKysely` request with method (`GET/POST/PATCH/PUT/DELETE`), path and URL query attributes. It is asynchronous: call `kysy(data)` and connect to `vastaus` / `lisaysVastaus` / `virhe` signals. Payloads are `QVariant` (maps/lists, JSON-shaped).
- Create requests with the global helper `kpk("/polku", KpKysely::GET)` (declared in `db/kirjanpito.h`).
- For SQLite, `sqlite/routes/*Route` classes (subclasses of `SQLiteRoute`) emulate the server endpoints (`/tositteet`, `/viennit`, `/kumppanit`, `/liitteet`, `/tilikaudet`, `/saldot`, `/myyntilaskut`, `/alv`, …) by translating them into SQL. They are registered in `SQLiteModel`'s constructor. **When changing or adding an endpoint, the SQLite route must match what the cloud server returns.** Schema is in `sqlite/luo.sql`.
- Some features (e.g. extras in `lisaosat/`, `pilvi/`) only exist with the cloud backend; check `oikeudet()` / the connection type before assuming availability.

### Global state

`Kirjanpito` (`db/kirjanpito.h`) is the singleton for the currently open book, reached via the global `kp()`. It exposes the connection (`yhteysModel()`), settings (`asetukset()`), chart of accounts (`tilit()`), fiscal periods (`tilikaudet()`), dimensions (`kohdennukset()`), etc. Most code reads shared state through `kp()->…`.

### Domain model

- `model/` — `Tosite` (voucher/document) with `TositeViennit` (entries) and `TositeRivit` (rows), `Lasku` (invoice), `Euro` (fixed-point money type — use it instead of `double` for amounts), `TositeLoki`.
- `kitsas.h` — key enums, notably `AlvKoodi` (VAT codes used throughout).
- `db/` — accounts (`Tili`), fiscal periods (`Tilikausi`), dimensions (`Kohdennus`), and their models.

### UI structure

`KitupiikkiIkkuna` is the main window; each top-level section is a `KitupiikkiSivu` subclass, organised by directory: `aloitussivu` (start page/login), `kirjaus` (entering vouchers), `apuri` (entry helpers incl. bank statement `tiliote`), `selaus` (browsing), `laskutus` (invoicing), `raportti` (reports), `alv` (VAT), `arkisto`/`arkistoija` (fiscal-year archive), `maaritys` (settings), `tilinpaatoseditori` (financial statements), `tuonti`/`pdftuonti` (imports: CSV, PDF, bank statements), `toimisto` (accounting-office features), `rekisteri` (customer/supplier registry), `tilikartat` (chart-of-accounts templates as `.kitsaskartta`).

### Localisation

UI chrome uses Qt `tr()` (`tr/kitsas_en.ts`, `tr/kitsas_sv.ts`). Bookkeeping content (account names, report texts, invoice texts) is multilingual via `kieli/` (`Kielet`, `Monikielinen`) and the `tulkkaa("…")` helper — use `tulkkaa` for strings that end up in the bookkeeping data or printed documents rather than the UI.
