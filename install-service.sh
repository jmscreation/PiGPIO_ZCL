#!/bin/bash

SERVICE_NAME="gpio-security"
SERVICE_INI_PATH="/etc/systemd/system/$SERVICE_NAME.service"
SERVICE_PATH="/usr/bin/$SERVICE_NAME-server"
SERVICE_WORKING_DIR="/etc/$SERVICE_NAME"
CONFIG_NAME="config.conf"

PROGRAM_PATH="program"
CONFIG_PATH="$CONFIG_NAME"

# Install as system service

read -r -d '' SERVICE_INI << EOM
[Unit]
Description=GPIO Security Controller Service
After=network.target
StartLimitBurst=12
StartLimitIntervalSec=30

[Service]
WorkingDirectory=$SERVICE_WORKING_DIR
ExecStart=$SERVICE_PATH

Type=simple
Restart=always
RestartSec=5
#User=

[Install]
WantedBy=multi-user.target

EOM

function info() {
	echo -e "\e[0;35m$1\e[0m"
}
function warn() {
	echo -e "\e[1;41m$1\e[0m"
}
function success() {
	echo -e "\e[4;32m$1\e[0m"
}

if [ ! -f "$PROGRAM_PATH" ]; then
 warn "Main program is missing! Please build/upload program then install again."
 exit 1
fi

if systemctl is-active --quiet "$SERVICE_NAME" ; then
 info "Stopping running service..."
# sudo pkill -f "$SERVICE_PATH"
 sudo service "$SERVICE_NAME" stop
fi

info "Installing service application..."
sudo cp "$PROGRAM_PATH" "$SERVICE_PATH"
sudo chmod ug+rx "$SERVICE_PATH"

info "Preparing service directory..."

if [ ! -d "$SERVICE_WORKING_DIR" ]; then
 info "Create service directory..."
 mkdir "$SERVICE_WORKING_DIR"
fi

if [ -f "$CONFIG_PATH" ]; then
 info "Copy config files..."
 cp "$CONFIG_PATH" "$SERVICE_WORKING_DIR/$CONFIG_NAME"
else
 warn "Missing config file!"
fi

info "Installing Service..."
echo "$SERVICE_INI"

# Write to service file
echo "$SERVICE_INI">"$SERVICE_INI_PATH"

info "Enable and Start the service daemon..."
sudo systemctl enable "$SERVICE_NAME"
sudo systemctl start "$SERVICE_NAME"

if [ -f "$SERVICE_INI_PATH" ] && systemctl is-enabled --quiet "$SERVICE_NAME" && systemctl is-active --quiet "$SERVICE_NAME"; then
 success "Service Installed Successfully!"
else
 warn "Service Failed To Install!"
fi
