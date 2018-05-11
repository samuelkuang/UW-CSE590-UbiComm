package com.ubicom.samuelkuang.steptrackerv2;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.graphics.*;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import static java.lang.System.currentTimeMillis;


/**
 * A simple {@link Fragment} subclass.
 * Activities that contain this fragment must implement the
 * to handle interaction events.
 * create an instance of this fragment.
 */
public class DisplayFragment extends Fragment {
    private static  final String TAG = "DisplayFragment";

    private TextView stepCoubterTV;
    private boolean _initialized = false;
    private ImageView imgTree;
    Paint mPaint = new Paint();

    private long _lastUpdateTime = 0;
    private int _lastCounter = 0;
    private int pace = 0;
    private int _initStepCounter = 0;
    private int _initSystemCounter = 0;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_display, container, false);

        stepCoubterTV = (TextView)view.findViewById(R.id.stepConterInfo);
        imgTree = (ImageView)view.findViewById(R.id.imgTree);

        mPaint.setColor(Color.BLUE);
        mPaint.setStyle(Paint.Style.STROKE);

        _initialized = true;

        // Inflate the layout for this fragment
        return view;
    }

    public void updateView(int stepCounter) {
        if (_initialized) {
            long curUpdateTime = currentTimeMillis();
            if (_lastUpdateTime > 0) {
                if (curUpdateTime > _lastUpdateTime && stepCounter > _lastCounter) {
                    int eclapse = (int) (curUpdateTime - _lastUpdateTime);
                    pace = (int) ((stepCounter - _lastCounter) * 60000 / (curUpdateTime - _lastUpdateTime));
                    _lastUpdateTime = curUpdateTime;
                    _lastCounter = stepCounter;
                }
            }
            else {
                _lastUpdateTime = curUpdateTime;
                _lastCounter = stepCounter;
            }

            String msg = "Steps counted: " + Integer.toString(stepCounter) +
                         " | @ " + Integer.toString(pace) + " SPM";
            stepCoubterTV.setText(msg);

            mPaint.setColor(Color.rgb(0, 255, Math.min((pace - 70) * 6, 255)));
            int depth = Math.min(stepCounter / 5, 11);

            drawComponent(depth);
        }
    }

    private void drawComponent(int depth) {
        Bitmap bmp = Bitmap.createBitmap(300,300, Bitmap.Config.ARGB_8888);

        Canvas canvas = new Canvas(bmp);
        drawTree(canvas, bmp.getWidth() /2, bmp.getHeight(), -90, depth);

        imgTree.setImageBitmap(bmp);
    }

    private void drawTree(Canvas canvas, int x1, int y1, double angle, int depth) {
        if (depth == 0)
            return;
        int x2 = x1 + (int) (Math.cos(Math.toRadians(angle)) * depth * 3.6);
        int y2 = y1 + (int) (Math.sin(Math.toRadians(angle)) * depth * 3.6);
        canvas.drawLine(x1, y1, x2, y2, mPaint);
        drawTree(canvas, x2, y2, angle - 20, depth - 1);
        drawTree(canvas, x2, y2, angle + 20, depth - 1);
    }

}
