#! /bin/sh

# This script is useful for messing with the keyboard grab code.
# Notably, the dialog is mouse-enabled and thus wllatex can be killed
# without using the keyboard.

while true; do
	dialog --msgbox "Click me" 10 20
	pkill wllatex
done
