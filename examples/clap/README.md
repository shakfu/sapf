# sapf CLAP plugin

A stereo CLAP instrument that runs sapf code. Built by default (`SAPF_BUILD_CLAP=ON`) as `build/examples/clap/sapf.clap`.

Install by copying `sapf.clap` to the CLAP folder:

| Platform | Folder |
|-|-|
| macOS | `~/Library/Audio/Plug-Ins/CLAP/` |
| Linux | `~/.clap/` |
| Windows | `%LOCALAPPDATA%\Programs\Common\CLAP\` |

To try it without a DAW, `make editor` opens the editor in `sapf_dev_host` (`dev_host.cpp`), a minimal host that plays to the default audio output.

## Input

**Editor (all platforms).** Dear ImGui in a pugl OpenGL view (`thirdparty/imgui`, `thirdparty/pugl`).

| Button | Key | Action |
|-|-|-|
| Eval Block | Cmd+Return (macOS), Ctrl+Return | Evaluate the selection; else the current line, or the whole multi-line `( )`, `[ ]` or `{ }` group it is part of |
| Eval All | Cmd+Shift+Return (macOS), Ctrl+Shift+Return | Evaluate the whole editor as one piece |
| Stop | Ctrl+. | Stop all sound. Cmd+. works only if the host passes it; AppKit usually consumes it |

Sounds add up by default. With **Replace** on, an evaluation's first `play` stops the sounds already playing; evaluating code that does not play, such as a definition, leaves them running. Replace also applies to UDP.

Output and errors appear in the log pane. **Clear** empties the editor and the log; it asks first when the editor has text, since that cannot be undone. **Wrap** (on by default) wraps long lines in both panes; evaluation still works on whole lines. On Linux the editor needs the host's CLAP timer support to redraw.

**UDP.** Each datagram to `127.0.0.1:57140` is evaluated; the reply is the output, or `ok`:

```
$ nc -u 127.0.0.1 57140
1 2 +
3
```

UDP is off by default; toggle it in the editor. `SAPF_CLAP_UDP_PORT` turns it on at load and sets the port. It binds to loopback only, but any local process can send it code.

The editor text and the Wrap, Replace and UDP settings are saved with the host project.

`sapf-prelude.txt` is compiled into the plugin and loaded at startup, since a DAW does not inherit `SAPF_PRELUDE` from the shell. Set `SAPF_PRELUDE` to load a different file instead.

## Limits

- One instance per process: the sapf engine is a process-wide singleton. A second instance fails to load.
- Output only; no audio input or MIDI from the host.
- Evaluation runs on its own thread. Code that never returns blocks removing the plugin.
- The editor is a plain text field: no syntax highlighting.

`test_clap_host.cpp` loads the built plugin headlessly and runs as ctest `example_clap_plugin`. `test_editor_logic.cpp` covers which code an eval request selects.
