#!/bin/sh
hey BeScreenCapture SET CaptureRect to "BRect(100,163,302,300)"
hey BeScreenCapture SET Scale to "float(75)"
hey BeScreenCapture SET RecordingTime to "int32(2000000)"
hey BeScreenCapture SET QuitWhenFinished to "bool(true)"
hey BeScreenCapture DO Record
