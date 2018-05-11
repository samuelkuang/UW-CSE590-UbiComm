package com.ubicom.samuelkuang.steptrackerv2;

import android.graphics.Color;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import com.jjoe64.graphview.GraphView;
import com.jjoe64.graphview.series.DataPoint;
import com.jjoe64.graphview.series.LineGraphSeries;

/**
 * A simple {@link Fragment} subclass.
 * Activities that contain this fragment must implement the
 * to handle interaction events.
 * create an instance of this fragment.
 */
public class DataFragment extends Fragment {
    private static  final String TAG = "DataFragment";

    private static int MAX_GRAPH_WIDTH = 300;

    private LineGraphSeries<DataPoint> _seriesRawX;

    private LineGraphSeries<DataPoint> _seriesSmoothSynth;
    private LineGraphSeries<DataPoint> _seriesMiddle;
    private LineGraphSeries<DataPoint> _seriesUpperPeak;
    private LineGraphSeries<DataPoint> _seriesLowerPeak;

    // smoothing accelerometer signal stuff
    private double _graph2LastXValue = 0d;

    private boolean _initialized = false;

    private int _initStepCounter = 0;
    private int _initSystemCounter = 0;

    private TextView textView;
    private Button btnReset;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_data, container, false);

        GraphView graph = (GraphView) view.findViewById(R.id.graph);
        _seriesRawX = new LineGraphSeries<>();
        graph.addSeries(_seriesRawX);
        graph.setTitle("RawAccelmeter Reading (Scrolling)");
        graph.getViewport().setMinX(0);
        graph.getViewport().setMaxX(MAX_GRAPH_WIDTH);
        graph.getViewport().setXAxisBoundsManual(true);

        GraphView graph2 = (GraphView) view.findViewById(R.id.graph2);
        _seriesSmoothSynth = new LineGraphSeries<>();
        _seriesSmoothSynth.setColor(Color.RED);
        graph2.addSeries(_seriesSmoothSynth);

        _seriesMiddle = new LineGraphSeries<>();
        _seriesMiddle.setColor(Color.YELLOW);
        graph2.addSeries(_seriesMiddle);
        _seriesUpperPeak = new LineGraphSeries<>();
        _seriesUpperPeak.setColor(Color.GREEN);
        graph2.addSeries(_seriesUpperPeak);
        _seriesLowerPeak = new LineGraphSeries<>();
        _seriesLowerPeak.setColor(Color.LTGRAY);
        graph2.addSeries(_seriesLowerPeak);
        graph2.setTitle("Smooth Accelmeter Reading (Scrolling)");
        graph2.getViewport().setMinX(0);
        graph2.getViewport().setMaxX(MAX_GRAPH_WIDTH);
        graph2.getViewport().setXAxisBoundsManual(true);

        textView = (TextView)view.findViewById(R.id.stepcounter);
        btnReset = (Button)view.findViewById(R.id.btnReset);
        btnReset.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                _initStepCounter = 0;
                _initSystemCounter = 0;
            }
        });

        _initialized = true;

        // Inflate the layout for this fragment
        return view;
    }

    public void updateView(float rawAccel,
                           float curAvg,
                           float upperPeak,
                           float lowerPeak,
                           float middle,
                           int stepCounter,
                           int systemStepCounter)
    {
        if (_initialized)
        {
            _seriesRawX.appendData(new DataPoint(_graph2LastXValue, rawAccel), true, MAX_GRAPH_WIDTH);
            _seriesSmoothSynth.appendData(new DataPoint(_graph2LastXValue, curAvg), true, MAX_GRAPH_WIDTH);
            _seriesMiddle.appendData(new DataPoint(_graph2LastXValue, middle), true, MAX_GRAPH_WIDTH);
            _seriesUpperPeak.appendData(new DataPoint(_graph2LastXValue, upperPeak), true, MAX_GRAPH_WIDTH);
            _seriesLowerPeak.appendData(new DataPoint(_graph2LastXValue, lowerPeak), true, MAX_GRAPH_WIDTH);
            _graph2LastXValue += 1d;

            if (_initStepCounter == 0) {
                _initStepCounter = stepCounter;
            }
            int curStepCounter = stepCounter - _initStepCounter;

            if (_initSystemCounter == 0) {
                _initSystemCounter = systemStepCounter;
            }
            int curSystemStepCounter = systemStepCounter - _initSystemCounter;

            String msg = "My counter: " + Integer.toString(curStepCounter) + " System step counter: " + Integer.toString(curSystemStepCounter);
            textView.setText(msg);
        }
    }
}
