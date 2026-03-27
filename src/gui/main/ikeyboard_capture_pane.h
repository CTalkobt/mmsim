#pragma once
#include <functional>

/**
 * Optional interface for plugin panes that provide a keyboard-capture toggle.
 * The host casts a live pane window to IKeyboardCapturePane* to connect the
 * pane's built-in toggle button to the frame's key-routing logic, without
 * requiring the host to include any plugin-specific headers.
 */
class IKeyboardCapturePane {
public:
    virtual ~IKeyboardCapturePane() {}

    /**
     * Register a callback invoked when the user presses the pane's own
     * "Capture Keyboard" toggle button.
     * @param fn  Called with true when capture is activated, false when released.
     */
    virtual void SetCaptureCallback(std::function<void(bool)> fn) = 0;

    /**
     * Synchronise the pane's toggle button state from an external source
     * (e.g. the toolbar button or Ctrl+Shift+K shortcut).
     */
    virtual void SetCaptureActive(bool active) = 0;
};
