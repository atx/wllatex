#! /bin/sh

# This script is useful for messing with the keyboard grab code.
# Notably, the dialog is mouse-enabled and thus wllatex can be killed
# without using the keyboard.

while true; do
	if not dialog --msgbox "Click me $RANDOM" 10 20; then
		exit
	fi
	pkill wllatex
done
