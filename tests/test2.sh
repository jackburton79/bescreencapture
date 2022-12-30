#!/bin/sh
hey BeScreenCapture SET CaptureRect to "BRect(100,163,302,300)"
hey BeScreenCapture SET Scale to 75
hey BeScreenCapture SET RecordingTime to 2
hey BeScreenCapture SET QuitWhenFinished to "bool(true)"
hey BeScreenCapture DO Record
