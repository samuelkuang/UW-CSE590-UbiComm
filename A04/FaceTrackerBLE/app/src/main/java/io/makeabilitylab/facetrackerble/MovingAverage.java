package io.makeabilitylab.facetrackerble;

public class MovingAverage {
    private static final int MAX_WINDOW_SIZE = 50;
    private int smoothingWindowSize = 20;

    private float _history[] = new float[MAX_WINDOW_SIZE];
    private float _runningTotal = 0;
    private float _curAvg = 0;
    private int _curIndex = 0;
    private int _count = 0;

    public MovingAverage(int windowSize) {
        smoothingWindowSize = Math.min(MAX_WINDOW_SIZE, windowSize);
        if (smoothingWindowSize <= 0) {
            smoothingWindowSize = 1;
        }
    }

    public float smooth(float value) {
        _runningTotal = _runningTotal - _history[_curIndex];
        _history[_curIndex] = value;
        _runningTotal = _runningTotal + _history[_curIndex];

        if (_count < smoothingWindowSize) {
            _count++;
        }

        _curIndex++;
        if (_curIndex >= smoothingWindowSize) {
            _curIndex = 0;
        }

        _curAvg = _runningTotal / _count;

        return _curAvg;
    }

    public float getCurrentAvg() {
        return _curAvg;
    }
}
