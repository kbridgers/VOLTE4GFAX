#!/bin/sh
# DSL Extended Script handling - Triggered from DSL CPE control Application with -a option
# Do not leave any blank lines in this script else you would get an error
# ": command not found"
#
[WaitForConfiguration]={
echo Processing WaitForConfiguration script section...
# Reconfigure System Interface settings to use PTM auto mode
sics 2 1 1 3
}
