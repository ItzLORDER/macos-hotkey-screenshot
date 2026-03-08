#!/bin/bash

set -e

APP_NAME="hotkey_daemon"
SOURCE="hotkey_daemon.cpp"
OUTPUT="./$APP_NAME"

echo "Building $APP_NAME with clang++..."

clang++ \
    -std=c++17 \
    -O2 \
    -o "$OUTPUT" \
    "$SOURCE" \
    -framework ApplicationServices \
    -framework Carbon \
    -framework CoreFoundation \
    -framework AppKit

echo "Build successful: $OUTPUT"
echo ""
echo "To run manually:  $OUTPUT"
echo "To add to Login Items, use the LaunchAgent setup below."
echo ""

# Optional: auto-setup LaunchAgent for login startup
read -p "Do you want to install as a LaunchAgent (auto-start on login)? [y/N] " REPLY
if [[ "$REPLY" =~ ^[Yy]$ ]]; then
    PLIST_DIR="$HOME/Library/LaunchAgents"
    PLIST_PATH="$PLIST_DIR/com.user.hotkey_daemon.plist"
    DAEMON_PATH="$(pwd)/$APP_NAME"

    mkdir -p "$PLIST_DIR"

    cat > "$PLIST_PATH" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>com.user.hotkey_daemon</string>
    <key>ProgramArguments</key>
    <array>
        <string>$DAEMON_PATH</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
    <key>KeepAlive</key>
    <true/>
    <key>StandardOutPath</key>
    <string>/tmp/hotkey_daemon.log</string>
    <key>StandardErrorPath</key>
    <string>/tmp/hotkey_daemon.err</string>
</dict>
</plist>
EOF

    echo "LaunchAgent installed: $PLIST_PATH"

    # Load immediately
    launchctl unload "$PLIST_PATH" 2>/dev/null || true
    launchctl load "$PLIST_PATH"
    echo "Daemon loaded and will start on next login automatically."
    echo "Logs: /tmp/hotkey_daemon.log | Errors: /tmp/hotkey_daemon.err"
fi
