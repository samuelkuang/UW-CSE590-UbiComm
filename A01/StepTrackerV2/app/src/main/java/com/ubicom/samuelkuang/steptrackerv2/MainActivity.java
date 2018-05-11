package com.ubicom.samuelkuang.steptrackerv2;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.design.widget.TabLayout;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;

import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;
import android.support.v4.view.ViewPager;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;

import android.widget.TextView;

public class MainActivity extends AppCompatActivity implements SensorEventListener{
    private SectionPagerAdapter mSectionPagerAdapter;

    private SensorManager _sensorManager;
    private Sensor _accelSensor;
    private Sensor _stepCounterSensor;

    private DataFragment mDataFragement;
    private DisplayFragment mDisplayFragment;

    private AccelDataAdapter dataAdapter = new AccelDataAdapter();

    private int initStepCounter = 0;
    private  int stepCounter = 0;

    private ViewPager mViewPager;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mSectionPagerAdapter = new SectionPagerAdapter(getSupportFragmentManager());

        mViewPager = (ViewPager)findViewById(R.id.container);
        setupViewPager(mViewPager);

        TabLayout tabLayout = (TabLayout)findViewById(R.id.tabs);
        tabLayout.setupWithViewPager(mViewPager);


        _sensorManager = (SensorManager) this.getSystemService(Context.SENSOR_SERVICE);
        _accelSensor = _sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
        _stepCounterSensor = _sensorManager.getDefaultSensor(Sensor.TYPE_STEP_COUNTER);

        _sensorManager.registerListener(this, _accelSensor, SensorManager.SENSOR_DELAY_GAME);
        _sensorManager.registerListener(this, _stepCounterSensor, SensorManager.SENSOR_DELAY_GAME);
    }

    private void setupViewPager(ViewPager viewPager) {
        SectionPagerAdapter adapter = new SectionPagerAdapter(getSupportFragmentManager());

        mDisplayFragment = new DisplayFragment();
        mDataFragement = new DataFragment();
        adapter.addFragment(mDisplayFragment, "Visualizer");
        adapter.addFragment(mDataFragement, "Debugger");

        viewPager.setAdapter(adapter);
        viewPager.setOffscreenPageLimit(0);
    }

    @Override
    public void onSensorChanged(SensorEvent sensorEvent) {
        switch(sensorEvent.sensor.getType()) {
            case Sensor.TYPE_ACCELEROMETER:
                dataAdapter.addDataEntry(sensorEvent.values);
                mDataFragement.updateView(
                        dataAdapter.getCurrentRaw(),
                        dataAdapter.getCurrentAverage(),
                        dataAdapter.getMiddleValue(),
                        dataAdapter.getUpperPeak(),
                        dataAdapter.getLowerPeak(),
                        dataAdapter.getStepCounter(),
                        stepCounter);
                mDisplayFragment.updateView(dataAdapter.getStepCounter());
                break;
            case Sensor.TYPE_STEP_COUNTER:
                if (initStepCounter < 1) {
                    initStepCounter = (int)sensorEvent.values[0];
                }
                stepCounter = (int)sensorEvent.values[0] - initStepCounter;
                break;
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {

    }
}
