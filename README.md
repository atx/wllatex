# wllatex
LaTeX IME for Wayland compositors. Work in progress, expect very rough edges.

<p align="center">
	<img src="https://github.com/atx/wllatex/blob/master/.github/images/output.gif">
</p>

## Building

```
meson build
ninja -C build
sudo ninja -C build install
```

## Usage

To start a persistent instance, run:

```
wllatex
```

As having the completion _everywhere_ can be annoying at times, one can
instead make a keybind for:
```
wllatex --oneshot
```

The program then automatically terminates after the (first one to receive at least
one input key event) text input field  loses focus.

## Controls

To enter the LaTeX mode, press the `\` key. The currently selected unicode
character is then displayed instead of the `\` character in the preedit buffer.

In the LaTeX mode, several keys have special handling:

 * `Tab` - Confirm the currently selected character if any, otherwise insert
 		   the text verbatim.
 * `Esc` - Reject the completion, inserting the text verbatim (note that `Esc` itself gets consumed).

Furthemore, any non-ascii, non-printable or whitespace character automatically
terminates the completion, inserting the accumulated text.

## Available symbols

The completion table is exactly the one available in the Julia REPL. See the
[extraction script](../master/steal_latex_from_julia.jl) for details.
