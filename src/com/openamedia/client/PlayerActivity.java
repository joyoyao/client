package com.openamedia.client;

import com.openamedia.player.APlayer;
import com.openamedia.player.APlayer.onAPlayerListener;

import android.support.v7.app.ActionBarActivity;
import android.text.method.ScrollingMovementMethod;
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.DragEvent;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnDragListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.ImageButton;
import android.widget.ProgressBar;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

public class PlayerActivity extends ActionBarActivity implements onAPlayerListener {
	private static final String TAG = "PlayerActivity";

	private TextView mText = null;
	
	private String mMediaPath = null;
	private SurfaceView mSV = null;
	private APlayer mPlayer = null;
	
	private ImageButton mPlayButton = null;
	private SeekBar mSeekBar = null;
	
	private State mState = State.STATE_IDLE;
	enum State {
		STATE_IDLE,
		STATE_INITIALIZED,
		STATE_PREPARED,
		STATE_PLAYING,
		STATE_PAUSED,
		STATE_SEEKING,
		STATE_STOPPED,
		STATE_EOS,
		STATE_ERROR,
	};
	
	private static final int MSG_UPDATE_POSITION = 0;
	private Handler mHandler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            switch(msg.what) {
            case MSG_UPDATE_POSITION:
            {
            	int position = mPlayer.getPosition();
                int duration = mPlayer.getDuration();
                
                //Log.e(TAG, "pos:"+position+"du:"+duration);

                if(duration != 0){
                	long pos = 1000L * position / duration;
                	mSeekBar.setProgress( (int) pos);
                }
                
                Message msg2 = mHandler.obtainMessage();
            	msg2.what = MSG_UPDATE_POSITION;
            	mHandler.sendMessageDelayed(msg2, 500);
            	break;
            }
            default:
            	break;
            }
        }
	};
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_player);
		
		mText = (TextView)this.findViewById(R.id.textView1);
		mText.setMovementMethod(ScrollingMovementMethod.getInstance());
		
		Intent intent = this.getIntent();
		mMediaPath = intent.getStringExtra(MainActivity.INTENT_KEY_MEDIA_PATH);

		mPlayButton = (ImageButton)this.findViewById(R.id.imageButton1);
		mPlayButton.setOnClickListener(new OnClickListener(){
			
			@Override
			public void onClick(View arg0) {
				if(mState == State.STATE_INITIALIZED){
					mPlayer.prepare();
				}else if(mState == State.STATE_PAUSED){
					mPlayer.play();
				}else if(mState == State.STATE_PLAYING){
					mPlayer.pause();
				}
			}
			
		});
		
		mSV = (SurfaceView)this.findViewById(R.id.surfaceView1);

		mPlayer = new APlayer();
		mPlayer.setListener(this);
		mPlayer.setMediaPath(mMediaPath);
		mSV.getHolder().addCallback(new SurfaceHolder.Callback(){

			@Override
			public void surfaceChanged(SurfaceHolder arg0, int arg1, int arg2,
					int arg3) {
				mPlayer.setVideoSink(arg0);
			}

			@Override
			public void surfaceCreated(SurfaceHolder arg0) {
				mPlayer.setVideoSink(arg0);
				mState = State.STATE_INITIALIZED;
				mPlayButton.setClickable(true);
			}

			@Override
			public void surfaceDestroyed(SurfaceHolder arg0) {
				// TODO Auto-generated method stub
			}
		});
		
		mSeekBar = (SeekBar)this.findViewById(R.id.seekBar1);
		mSeekBar.setMax(1000);
		mSeekBar.setOnSeekBarChangeListener(new OnSeekBarChangeListener(){

			@Override
			public void onProgressChanged(SeekBar arg0, int arg1, boolean arg2) {
	            if (!arg2) {
	                // We're not interested in programmatically generated changes to
	                // the progress bar's position.
	                return;
	            }
				
				int duration = mPlayer.getDuration();
	            int newposition = (duration * arg1) / 1000;
	            mPlayer.seek(newposition);
			}

			@Override
			public void onStartTrackingTouch(SeekBar arg0) {
				// TODO Auto-generated method stub
				
			}

			@Override
			public void onStopTrackingTouch(SeekBar arg0) {
				// TODO Auto-generated method stub
				
			}
		});
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {

		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.player, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		/*
		int id = item.getItemId();
		if (id == R.id.action_settings) {
			return true;
		}
		*/
		return super.onOptionsItemSelected(item);
	}
	
	@Override 
	public boolean onKeyDown(int keyCode, KeyEvent event) {		
		if (keyCode == KeyEvent.KEYCODE_BACK){
			mHandler.removeMessages(MSG_UPDATE_POSITION);
			mPlayer.release();
			mPlayer = null;
		}
				
		return super.onKeyDown(keyCode, event);
	}
	
	@Override
	public void onVideoSizeChanged(int width, int height) {
		LayoutParams lp = mSV.getLayoutParams();
		lp.width = width;
		lp.height = height;
		mSV.setLayoutParams(lp);
		//mSV.getHolder().setFixedSize(width, height);
		mSV.requestLayout();
	}

	@Override
	public void onLog(String log) {
		mText.append(log + "\n");
	}

	@Override
	public void onSetListenerDone() {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void onSetPathDone() {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void onSetVideoSinkDone() {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void onPrepareDone() {
		mPlayer.play();
	}

	@Override
	public void onPlayDone() {
    	Message msg = mHandler.obtainMessage();
    	msg.what = MSG_UPDATE_POSITION;
    	mHandler.sendMessage(msg);
		
		mState = State.STATE_PLAYING;
		mPlayButton.setBackgroundResource(R.drawable.controller_pause);
	}

	@Override
	public void onSeekDone() {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void onPauseDone() {
		mHandler.removeMessages(MSG_UPDATE_POSITION);
		
		mState = State.STATE_PAUSED;
		mPlayButton.setBackgroundResource(R.drawable.controller_play);
	}

	@Override
	public void onError() {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void onPlaybackCompleted() {
		// TODO Auto-generated method stub
		Log.v(TAG, "onPlaybackCompleted!!!!!!!!!!!!!!!!!");
		mHandler.removeMessages(MSG_UPDATE_POSITION);
		mPlayer.seek(0);
		mPlayer.pause();
		mSeekBar.setProgress(0);
		
		mState = State.STATE_PAUSED;
		mPlayButton.setBackgroundResource(R.drawable.controller_play);
	}

}
