# Local Weather Style

Styling contract for the `ccal` GUI, written for the `render_weather` MCP tool.
Read this file before applying a weather restyle, then map the tool's directives
onto the anchors listed below.

## 1. Application Architecture

| Aspect | Detail |
| --- | --- |
| Language | C (C99), no framework |
| GUI layer | Win32 API directly (`ccal_gui.c`) |
| Engine layer | `ccal.c`, shared with the CLI front end |
| Modules | `modules/converter.c` for unit conversion |
| GUI build | `gcc -DBUILDING_GUI ccal_gui.c ccal.c ccal_gui.res -o ccal_gui.exe -mwindows` |
| Resources | `ccal_gui.rc` compiled with `windres` to `ccal_gui.res` |

There is **no stylesheet**. Every visual property is a literal in C source, so a
restyle is a source edit plus a recompile. All styling lives in `ccal_gui.c`.

### Application context: not game-like

`ccal` is a calculator - a utility form with an input box, a result label, a
button grid, and a history panel. It is **not game-like**.

Per the `render_weather` directive tree, the asset branch is therefore skipped:
**do not copy `cloud.svg`, `rain.svg`, `snow.svg`, `sun.svg`, or `moon.svg`**
into this repo. Weather is expressed through palette and background treatment
only. Re-evaluate this only if `ccal` gains a game-like surface.

## 2. Styling Anchors

These are the only places colour and type are decided. A restyle touches these
and nothing else.

| # | Anchor | What it controls | Original value |
| --- | --- | --- | --- |
| 1 | `wc.hbrBackground` in `WinMain` | Main window backdrop | `COLOR_WINDOW + 1` (system) |
| 2 | `histWc.hbrBackground` in `WinMain` | History panel class backdrop | `COLOR_WINDOW + 1` (system) |
| 3 | `DrawHistory()` background fill | History panel body | `RGB(245, 245, 245)` |
| 4 | `DrawHistory()` text colour | History entry text | `RGB(0, 0, 0)` |
| 5 | `DrawHistory()` hover fill | Hovered history row | `RGB(220, 235, 255)` |
| 6 | `DrawHistory()` hover border | Hovered row outline | `RGB(100, 150, 200)` |
| 7 | `DrawHistory()` divider pen | Column dividers | `RGB(47, 79, 79)` |
| 8 | `AddButton()` style flags | Button paint path | `BS_PUSHBUTTON` (system themed) |
| 9 | `hSmallFont` in `WM_CREATE` | "Clear Hist." button face | `MS Shell Dlg`, height `-10` |
| 10 | `WM_CTLCOLOR*` handlers | Input box and result label | absent originally |
| 11 | `WM_ERASEBKGND` handler | Backdrop texture | absent originally |

Anchors 8, 10, and 11 did not exist before the first weather restyle. They were
added so buttons, edit controls, and the backdrop become styleable at all:

- Buttons switched to `BS_OWNERDRAW`, painted in `WM_DRAWITEM`.
- `WM_CTLCOLOREDIT` / `WM_CTLCOLORSTATIC` colour the input and result.
- `WM_ERASEBKGND` paints the backdrop and any precipitation texture.

## 3. Palette Contract

Define every colour as a `PAL_*` macro in the palette block at the top of
`ccal_gui.c`. Never inline an `RGB()` literal anywhere else - a restyle should
only ever rewrite that one block.

| Macro | Role |
| --- | --- |
| `PAL_BACKDROP` | Main window backdrop |
| `PAL_PANEL` | History panel body |
| `PAL_TEXT` | Primary text |
| `PAL_TEXT_ON_ACCENT` | Text drawn on an accent fill |
| `PAL_HOVER` | Hovered history row fill |
| `PAL_HOVER_EDGE` | Hovered row outline |
| `PAL_DIVIDER` | Column dividers, button outlines |
| `PAL_BTN_FACE` | Digit button face |
| `PAL_BTN_OP` | Operator button face |
| `PAL_BTN_ACCENT` | `=` button face |
| `PAL_BTN_PRESSED` | Any button while held |
| `PAL_FIELD` | Input box and result label background |
| `PAL_PRECIP` | Precipitation texture stroke |

Brushes are created once in `WM_CREATE`, cached in file-scope `HBRUSH`
globals, and released in `WM_DESTROY`. Do not create a brush per paint call
except where the original code already did so inside `DrawHistory()`.

## 4. Mapping Directives To The Palette

`render_weather` returns three inputs that matter here.

### Render Background: `day` | `night`

Sets overall lightness. `day` keeps a light panel and a mid-tone backdrop;
`night` drops the backdrop and panel several steps darker and lifts `PAL_TEXT`
toward near-white so contrast survives.

### Render Tone: `hot` | `medium` | `cold`

Sets hue family.

| Tone | Hue family | Accent direction |
| --- | --- | --- |
| `hot` | ember orange, terracotta, warm ochre | saturated warm accents |
| `medium` | neutral slate and putty | low-saturation accents |
| `cold` | glacier blue, pale steel | cool desaturated accents |

### Precipitation Use: `stormy` | `cloudy` | `sunny`

Sets saturation, contrast, and backdrop texture.

| Precipitation | Treatment |
| --- | --- |
| `sunny` | full saturation, bright backdrop, no texture |
| `cloudy` | pull saturation down, flatten contrast, no texture |
| `stormy` | pull saturation down, deepen the backdrop, and draw faint diagonal `PAL_PRECIP` streaks in `WM_ERASEBKGND` |

Tone and precipitation compose: `hot` + `stormy` is a warm ember palette that
has been dimmed and cooled by overcast, not a bright orange one.

## 5. Contrast Floor

Every restyle must keep text legible. Before compiling, check that:

- `PAL_TEXT` against `PAL_PANEL` and `PAL_FIELD` stays near or above a 4.5:1
  contrast ratio.
- `PAL_TEXT_ON_ACCENT` against `PAL_BTN_ACCENT` clears the same bar.
- Precipitation streaks stay within a few steps of `PAL_BACKDROP`. They are
  texture, not content, and must never compete with the button labels.

If a weather combination would breach the floor, move the *background* rather
than the text - the weather reads through the backdrop, not through the copy.

## 6. Procedure For The Next Call

1. Check out or create the `local-weather` branch.
2. Read this file.
3. Rewrite only the `PAL_*` block in `ccal_gui.c` for the new directives.
4. Set the precipitation texture flag to match the directive.
5. Rebuild:

   ```bash
   windres ccal_gui.rc -O coff -o ccal_gui.res
   gcc -DBUILDING_GUI ccal_gui.c ccal.c ccal_gui.res -o ccal_gui.exe -mwindows
   ```

6. Launch `ccal_gui.exe` and confirm the contrast floor holds.

Because only the palette block changes, successive restyles stay a small diff
and the layout never drifts.
