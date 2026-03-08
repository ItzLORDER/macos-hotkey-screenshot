#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <iostream>
#include <unistd.h>

// Simulate: Command + Shift + Control + 4
void simulateScreenshotHotkey() {
    CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
    if (!source) return;

    // Key down: Cmd + Shift + Ctrl + 4
    CGEventRef keyDown = CGEventCreateKeyboardEvent(source, (CGKeyCode)21, true); // keycode 21 = '4'
    CGEventSetFlags(keyDown,
        kCGEventFlagMaskCommand |
        kCGEventFlagMaskShift |
        kCGEventFlagMaskControl
    );
    CGEventPost(kCGHIDEventTap, keyDown);
    CFRelease(keyDown);

    usleep(50000); // 50ms

    // Key up
    CGEventRef keyUp = CGEventCreateKeyboardEvent(source, (CGKeyCode)21, false);
    CGEventSetFlags(keyUp,
        kCGEventFlagMaskCommand |
        kCGEventFlagMaskShift |
        kCGEventFlagMaskControl
    );
    CGEventPost(kCGHIDEventTap, keyUp);
    CFRelease(keyUp);

    CFRelease(source);
}

// CGEvent tap callback
CGEventRef eventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void* userInfo) {
    if (type == kCGEventKeyDown) {
        CGEventFlags flags = CGEventGetFlags(event);
        CGKeyCode keyCode = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);

        // keycode 1 = 'S'
        if (keyCode == 1) {
            bool hasShift   = (flags & kCGEventFlagMaskShift)     != 0;
            bool hasOption  = (flags & kCGEventFlagMaskAlternate)  != 0;
            bool hasControl = (flags & kCGEventFlagMaskControl)    != 0;
            bool hasCommand = (flags & kCGEventFlagMaskCommand)    != 0;

            // Shift + Option + S
            bool combo1 = hasShift && hasOption  && !hasControl && !hasCommand;
            // Shift + Control + S
            bool combo2 = hasShift && hasControl && !hasOption  && !hasCommand;
            // Shift + Command + S
            bool combo3 = hasShift && hasCommand && !hasOption  && !hasControl;

            if (combo1 || combo2 || combo3) {
                simulateScreenshotHotkey();
                // Consume the original event
                return NULL;
            }
        }
    }

    // Re-enable tap if it gets disabled
    if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
        CGEventTapEnable((CFMachPortRef)userInfo, true);
    }

    return event;
}

int main() {
    // Check Accessibility permissions (pure C API, no ObjC)
    const void* keys[]   = { kAXTrustedCheckOptionPrompt };
    const void* values[] = { kCFBooleanTrue };
    CFDictionaryRef options = CFDictionaryCreate(
        kCFAllocatorDefault, keys, values, 1,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks
    );
    bool trusted = AXIsProcessTrustedWithOptions(options);
    CFRelease(options);
    if (!trusted) {
        std::cerr << "[HotkeyDaemon] Accessibility access required. Please grant in System Settings -> Privacy & Security -> Accessibility.\n";
        // Don't exit — macOS will show the prompt; wait a moment then retry
        sleep(3);
    }

    CGEventMask eventMask = CGEventMaskBit(kCGEventKeyDown);

    CFMachPortRef eventTap = CGEventTapCreate(
        kCGSessionEventTap,
        kCGHeadInsertEventTap,
        kCGEventTapOptionDefault,
        eventMask,
        eventTapCallback,
        NULL
    );

    if (!eventTap) {
        std::cerr << "[HotkeyDaemon] Failed to create event tap. Make sure Accessibility permission is granted.\n";
        return 1;
    }

    // Pass the tap ref as userInfo so we can re-enable on timeout
    CGEventTapEnable(eventTap, true);

    CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);

    std::cout << "[HotkeyDaemon] Running. Listening for Shift+Option+S / Shift+Control+S / Shift+Cmd+S\n";

    CFRunLoopRun();

    CFRelease(runLoopSource);
    CFRelease(eventTap);
    return 0;
}
