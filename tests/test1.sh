#!/bin/sh
hey BeScreenCapture SET CaptureRect to "BRect(100,100,300,300)"
hey BeScreenCapture SET Scale to 100
hey BeScreenCapture SET RecordingTime to 5
hey BeScreenCapture SET QuitWhenFinished to "bool(true)"
hey BeScreenCapture DO Record
