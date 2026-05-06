#pragma once

struct SDL_Window;

namespace platform {

// Tint the OS-drawn title bar to match the app theme.
// rgb in [0,1]; dark_appearance picks light/dark text and traffic-light style.
// macOS: NSWindow backgroundColor + appearance.
// Windows: DwmSetWindowAttribute (Win10 1809+ for dark mode, Win11 22000+ for color).
// Linux/other: no-op.
void set_titlebar(SDL_Window* w, float r, float g, float b, bool dark_appearance);

} // namespace platform
