# Gestion des Ventes - C++ / Qt Widgets

A desktop implementation of the supplied French sales-dashboard reference.
All interface elements are built in C++; there is no HTML, web view, QML,
Designer `.ui` file, or external image dependency.

## Date-only sorting update

For an existing copy of the original project, replace only `mainwindow.cpp`
and `mainwindow.h` with the versions in this archive, then rebuild and run.
Back up any local edits first. No other source file or project setting changed.

## Open the project in Qt Creator

1. Extract the whole ZIP into a normal folder. Do not open source files inside the ZIP.
2. In Qt Creator, open `GestionVentesQt/CMakeLists.txt` as a project.
3. Select an installed **Desktop Qt 6** kit, with its matching MinGW or MSVC compiler.
4. Configure the project and select **Run** (green triangle).

The project is written for Qt 6 and uses APIs also available in Qt 5.15.
`GestionVentes.pro` is included as an alternative qmake project for a Qt 5.15
or Qt 6 desktop kit. Open **either** the CMake file **or** the `.pro` file,
not both at once. Qt Creator alone is not a Qt development kit: the kit must
also include Qt libraries, headers, and a compiler.

Only the **Widgets** Qt module needs to be specified in the project. Its
Core and GUI dependencies provide the drawing and PDF APIs used here.
There is **no Qt Charts, Qt SVG, Qt PrintSupport, SQL, or Oracle dependency**.

## What is included

| Reference area | Implementation |
| --- | --- |
| Blue header | Cube icon, French title, responsible-sales menu |
| Left form | Date, client completion, product, quantity, status |
| Derived fields | Available stock, FEFO lot selection, amount, validation badge |
| Order list | Eight demo orders, colored status badges, edit/delete buttons |
| Toolbar | Case-insensitive search, sorting, status/client filters, PDF export |
| Pagination | Five orders per page, previous/next, up to five page-number buttons |
| Statistics | Delivered quantities by client/month, available stock vs minimum |
| Diagnostic card | Low-stock warnings and eligible lots expiring within 30 days |

The initial order list is sorted by **date, newest first**. The **Trier** menu
offers only **Date : plus recente** and **Date : plus ancienne**. ID, amount
and client are no longer available as sort choices. Orders with identical
dates retain the existing descending-ID tie-break for a deterministic order.
The application opens at 1220 x 850 logical pixels and uses layouts rather than
absolute positions. Native font rendering and system dialogs differ by platform;
this is an implementation of the design, not a guaranteed pixel-identical image.

## Try it

### Add a draft

Type a client, choose an article, enter a positive quantity, keep **Brouillon**,
and press **+ Ajouter**. The row is added; no inventory is reserved.
The form's calculated price is `quantity * unit price`, rounded to the nearest cent.

### Reserve or deliver inventory

Choose **Planifiee** or **Livree**. The preview selects the earliest-expiring
eligible lots. A quantity can be split across multiple lots; hover over the lot
field or table cell to see the breakdown. The save is rejected when eligible
stock is insufficient. Blocked or expired lots never cover a shortage.

### Edit or delete

Click the blue pencil, or double-click a table row, to load it into the form.
The save button becomes **Enregistrer**. The **Annuler** form button abandons
editing without changing the order. The red trash icon asks for confirmation.

### Filter and export

Search is combined with both dropdown filters. **Exporter PDF** exports the
entire filtered/sorted result, including orders on other pages. Large results
continue across PDF pages with repeated headers. The existing output file is
replaced only after the new PDF has been written successfully.

### Read the charts

The first two charts follow the **article selected in the left form** and count
only **delivered** orders. They are intentionally independent of the table's
search filters. This avoids adding liters, kilograms and pieces together.
The client chart shows up to five largest clients, and the month chart shows up
to six latest months with deliveries. The stock chart labels each product's unit.

The monthly demo values are calculated from the provided demo orders, rather
than copying numbers that do not reconcile with those orders in the reference.
Initial water totals are 200 L, 180 L and 20 L for August, September and October.
Available stock starts at 450 L of water, 90 kg of chlorine and 350 bottles.

## Source map

```
GestionVentesQt/
  CMakeLists.txt            CMake project; prefers Qt 6, supports Qt 5.15
  GestionVentes.pro         Alternative qmake project
  main.cpp                 QApplication, theme loading, window startup
  mainwindow.h/.cpp        Layouts, controls, table, signals and user actions
  salesstore.h/.cpp        Demo products, lots, orders, validation and FEFO
  barchartwidget.h/.cpp    Reusable chart widget implemented with QPainter
  pdfexporter.h/.cpp       Filtered-order PDF export with pagination
  resources.qrc            Bundles the stylesheet into the executable
  styles/app.qss           Colors, typography, borders, badges and spacing
  tests/salesstore_tests.cpp  Optional model checks, without Qt Test
```

Start with `MainWindow::MainWindow()` and these four UI-building functions:
`createHeader()`, `createFormPanel()`, `createOrdersPanel()`, and
`createStatisticsPanel()`. Connections between buttons/fields and actions are
in `connectSignals()`.

To change the blue theme, edit `styles/app.qss`. To change products, prices,
thresholds or example orders, edit `SalesStore::seedDemoData()`. The currency
label is deliberately **DH**, as in the reference; change `formatMoney()` to
use another label. Prices are demo data, not current market prices.

## Command-line build

From a terminal configured for the installed Qt kit:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH="<path-to-your-Qt-kit>"
cmake --build build --config Release
```

Replace the prefix placeholder with the actual kit folder (the directory that
contains `bin`, `include`, and `lib`). Qt Creator normally supplies it itself.
Use a compiler that matches the kit; an MSVC Qt library cannot be linked with
MinGW, or vice versa. For MinGW, use a MinGW-configured terminal/generator.

For an installed qmake kit, an alternative is:

```sh
qmake GestionVentes.pro
```

Then run the kit's build tool: `mingw32-make`, `nmake`, or `make` as appropriate.

## Optional tests

```sh
cmake -S . -B build -DSALES_BUILD_TESTS=ON -DCMAKE_PREFIX_PATH="<path-to-your-Qt-kit>"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

These checks cover initial stock, multi-lot FEFO, future-date expiry, blocked
and expired lots, reserving stock, atomic failed edits, status transitions,
quantity validation, money formatting, and deleting orders. They do not test
the appearance of the GUI or PDF output.

## Scope and limitations

**This is an in-memory demonstration, not a production sales/accounting system.**
Orders reset to the demo set when the app closes. No database, authentication,
persistent audit trail, invoice tax calculation, or supplier purchasing is
implemented. The account menu displays an About dialog; it is not a login.

In this demo, planned and delivered allocations are both deducted from a lot's
opening quantity. Changing or deleting a delivered order releases that allocation.
A production system should instead keep immutable stock movements and handle
returns/corrections explicitly, with authorization, persistence and transactions.

Order dates are seeded in 2026; lot expiry dates are relative to the day you run
the app so the example warnings remain useful. The form defaults to today's date.
The model treats a lot as eligible through its expiry date and excludes it after
that date. Both the order date and today's date are considered for new allocations.

**Verification:** source structure, resource references and packaging were checked.
A Qt development kit was not available in the generation environment, so the
C++ build, model tests, GUI rendering and PDF output were not executed there.
Build and run the project locally before relying on it.

## Official API references

- Qt Widgets: https://doc.qt.io/qt-6/qtwidgets-index.html
- Qt widget startup/layout tutorial: https://doc.qt.io/qt-6/widgets-tutorial.html
- Qt Creator project opening: https://doc.qt.io/qtcreator/creator-project-opening.html
- QPainter: https://doc.qt.io/qt-6/qpainter.html
- QPdfWriter: https://doc.qt.io/qt-6/qpdfwriter.html
- CMake with Qt: https://doc.qt.io/qt-6/cmake-get-started.html
