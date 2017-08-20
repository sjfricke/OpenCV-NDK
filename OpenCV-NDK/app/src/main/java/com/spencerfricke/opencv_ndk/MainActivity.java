package com.spencerfricke.opencv_ndk;

import android.Manifest;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.support.v4.app.ActivityCompat;
import android.os.Bundle;
import android.support.v7.app.ActionBarActivity;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;

public class MainActivity extends ActionBarActivity {

    static {
        System.loadLibrary("opencv_ndk");
    }

    static final String TAG = "OpenCV-NDK-Java";

    private static final int PERMISSION_REQUEST_CODE_CAMERA = 1;

    Button mStartTestButton;
    SurfaceView mSurfaceView;
    SurfaceHolder mSurfaceHolder;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Asking for permissions
        String[] accessPermissions = new String[] {
                Manifest.permission.CAMERA
        };
        boolean needRequire = false;
        for(String access : accessPermissions) {
            int curPermission = ActivityCompat.checkSelfPermission(this, access);
            if (curPermission != PackageManager.PERMISSION_GRANTED) {
                needRequire = true;
                break;
            }
        }
        if (needRequire) {
            ActivityCompat.requestPermissions(
                    this,
                    accessPermissions,
                    PERMISSION_REQUEST_CODE_CAMERA);
            return;
        }

        // send class activity and assest fd to native code
        onCreateJNI(this, getAssets());

        // set up the Surface to display images too
        mSurfaceView = (SurfaceView) findViewById(R.id.surfaceView);
        mSurfaceHolder = mSurfaceView.getHolder();

        mStartTestButton = (Button)findViewById(R.id.start_button);
        mStartTestButton.setOnClickListener(startTestListener);

        mSurfaceHolder.addCallback(new SurfaceHolder.Callback() {

            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                Log.v(TAG, "surfaceChanged format="+format+", width="+width+", height="+ height);
            }

            public void surfaceCreated(SurfaceHolder holder) {
                Log.v(TAG, "surfaceCreated");
                setSurface(holder.getSurface());
            }

            public void surfaceDestroyed(SurfaceHolder holder) {
                Log.v(TAG, "surfaceDestroyed");
            }

        });
    }

    private View.OnClickListener startTestListener = new View.OnClickListener() {
        @Override
        public void onClick(View view) {
            startCV();
        }
    };

    public native void onCreateJNI(Activity callerActivity, AssetManager assetManager);

    public native void startCV();

    // Sends surface buffer to NDK
    public native void setSurface(Surface surface);
}
