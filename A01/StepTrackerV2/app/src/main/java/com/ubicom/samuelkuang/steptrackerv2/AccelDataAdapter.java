package com.ubicom.samuelkuang.steptrackerv2;

public class AccelDataAdapter {
    private static final String TAG = "AccelDataSingleton";

    private static int SMOOTHING_WINDOW_SIZE = 15;

    // smoothing accelerometer signal stuff
    private MovingAverage _AccelAvg = new MovingAverage(SMOOTHING_WINDOW_SIZE);       // Accelerator reading average and history
    private MovingAverage _upperPeaks = new MovingAverage(3);       // upper peak and history
    private MovingAverage _lowerPeaks = new MovingAverage(3);       // lower peak and history

    private float _lastAccelAvg = 0;
    private float _rawAccelValue = 0;
    private boolean _goingdown = false;
    private int stepCounter = 0;
    private long _lastPeakTime = 0;
    private boolean counted = true;

    public AccelDataAdapter() { }

    public void addDataEntry(float[] accelValues) {
        _rawAccelValue = Math.abs(accelValues[0]) + Math.abs(accelValues[1]) + Math.abs(accelValues[2]);

        float _curAccelAvg = _AccelAvg.smooth(_rawAccelValue);

        // detect peak
        if (_curAccelAvg < _lastAccelAvg) {
            // going down
            if (!_goingdown && _lastAccelAvg - _lowerPeaks.getCurrentAvg() > 0.6) {
                // Upper peak confirmed.
                _goingdown = true;
                _upperPeaks.smooth(_curAccelAvg);

                long curTime = System.currentTimeMillis();
                int timedif = (int)(curTime - _lastPeakTime);

                if (!counted && timedif > 350 && timedif < 2500 ) {
                    stepCounter++;
                    counted = true;
                }

                _lastPeakTime = curTime;
            }
        }
        else if (_curAccelAvg > _lastAccelAvg)  {
            // going up
            if (_goingdown && _upperPeaks.getCurrentAvg() - _lastAccelAvg > 0.6) {
                // Lower peak confirmed.
                _goingdown = false;
                _lowerPeaks.smooth(_curAccelAvg);
                if (counted) {
                    counted = false;
                }
            }
        }
        else {
            // No status change.
        }

        _lastAccelAvg = _curAccelAvg;
    }

    public int getStepCounter() {
        return stepCounter;
    }

    public float getCurrentAverage() {
        return _AccelAvg.getCurrentAvg();
    }

    public float getCurrentRaw() {
        return _rawAccelValue;
    }

    public float getMiddleValue() {
        return (_lowerPeaks.getCurrentAvg() + _upperPeaks.getCurrentAvg()) / 2;
    }

    public float getUpperPeak() {
        return _upperPeaks.getCurrentAvg();
    }

    public float getLowerPeak() {
        return _lowerPeaks.getCurrentAvg();
    }
}
