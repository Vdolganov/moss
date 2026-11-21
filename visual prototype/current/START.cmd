@echo off
cd /d "%~dp0"
start "MOSS Interface Lab" /min node server.mjs
start "" http://127.0.0.1:4174
