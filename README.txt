% CortexView2: continuous optical imaging with PVCam and NIDaqmx for timing
% Keji Li
% 2015 September

This software in principle should work with all photo-diode-synced optical imaging systems using PVCam supported cameras and NIDaqmx driven analog input for the photo-diode and camera ♀

# Configuration Options

## Camera Clear Mode

Dark current accumulates on CCD, and this camera doesn't have a 

## Diode signal levels

These are the cutoffs for different levels of diode signals.
The six levels of monitor sync signal output have been relatively linearized.
If you change the monitor-diode fit curve in the diode_signal_fit.mat, you need to change the cutoffs in the config file as well.
