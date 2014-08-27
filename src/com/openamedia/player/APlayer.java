package com.openamedia.player;

import java.io.UnsupportedEncodingException;
import java.lang.ref.WeakReference;

import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;


public class APlayer {
	private static final String TAG = "APlayer";
	
	private Handler mEventHandler = null;
	
	private onAPlayerListener mListener = null;
	public interface onAPlayerListener {
		public void onVideoSizeChanged(int width, int height);
		public void onLog(String log);
		public void onSetListenerDone();
		public void onSetPathDone();
		public void onSetVideoSinkDone();
		public void onPrepareDone();
		public void onPlayDone();
		public void onSeekDone();
		public void onPauseDone();
		public void onError();
		public void onPlaybackCompleted();
	}
	
	private class EventHandler extends Handler {
        private APlayer mPlayer = null;

        public EventHandler(APlayer ap, Looper looper) {
            super(looper);
            mPlayer = ap;
        }

    	private static final int NATIVE_MSG_NOTIFY_SET_LISTENER_DONE = 2;
    	private static final int NATIVE_MSG_NOTIFY_SET_PATH_DONE = 3;
    	private static final int NATIVE_MSG_NOTIFY_SET_VIDEO_SINK_DONE = 4;
    	private static final int NATIVE_MSG_NOTIFY_PREPARE_DONE = 5;
    	private static final int NATIVE_MSG_NOTIFY_PLAY_DONE = 6;
    	private static final int NATIVE_MSG_NOTIFY_SEEK_DONE = 7;
    	private static final int NATIVE_MSG_NOTIFY_PAUSE_DONE = 8;
    	private static final int NATIVE_MSG_NOTIFY_ERROR = 9;
    	private static final int NATIVE_MSG_NOTIFY_EOS = 10;
    	
        @Override
        public void handleMessage(Message msg) {
            switch(msg.what) {
            case NATIVE_MSG_NOTIFY_VIDEO_SIZE:
            	if(mListener != null)
            		mListener.onVideoSizeChanged(msg.arg1, msg.arg2);
            	break;
            case NATIVE_MSG_NOTIFY_LOG_INFO:
            	if(mListener != null){
            		byte[] bytes = (byte[])msg.obj;
            		String str = null;
					try {
						str = new String(bytes,"UTF-8");
						mListener.onLog(str);
					} catch (UnsupportedEncodingException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
            	}
            	break;
            case NATIVE_MSG_NOTIFY_SET_LISTENER_DONE:
            	if(mListener != null)
            		mListener.onSetListenerDone();
            	break;
            case NATIVE_MSG_NOTIFY_SET_PATH_DONE:
            	if(mListener != null)
            		mListener.onSetPathDone();
            	break;
            case NATIVE_MSG_NOTIFY_SET_VIDEO_SINK_DONE:
            	if(mListener != null)
            		mListener.onSetVideoSinkDone();
            	break;
            case NATIVE_MSG_NOTIFY_PREPARE_DONE:
            	if(mListener != null)
            		mListener.onPrepareDone();
            	break;
            case NATIVE_MSG_NOTIFY_PLAY_DONE:
            	if(mListener != null)
            		mListener.onPlayDone();
            	break;
            case NATIVE_MSG_NOTIFY_SEEK_DONE:
            	if(mListener != null)
            		mListener.onSeekDone();
            	break;
            case NATIVE_MSG_NOTIFY_PAUSE_DONE:
            	if(mListener != null)
            		mListener.onPauseDone();
            	break;
            case NATIVE_MSG_NOTIFY_ERROR:
            	if(mListener != null)
            		mListener.onError();
            	break;
            case NATIVE_MSG_NOTIFY_EOS:
            	if(mListener != null)
            		mListener.onPlaybackCompleted();
            default:
            	break;
            }
        }
	}
	
	public APlayer(){
        Looper looper;
        if ((looper = Looper.myLooper()) != null) {
            mEventHandler = new EventHandler(this, looper);
        } else if ((looper = Looper.getMainLooper()) != null) {
            mEventHandler = new EventHandler(this, looper);
        } else {
            mEventHandler = null;
        }

        nativeSetup(new WeakReference<APlayer>(this));
	}
	
	public void setMediaPath(String path){
		nativeSetPath(path);
	}
	
	public void setVideoSink(SurfaceHolder holder){
		nativeSetSurface(holder.getSurface());
	}
	
	public void setListener(onAPlayerListener listener){
		mListener = listener;
	}
	
	public void prepare(){
		nativePrepare();
	}
	
	public void play(){
		nativePlay();
	}
	
	public void seek(int time){
		nativeSeek(time);
	}
	
	public void pause(){
		nativePause();
	}
	
	public void stop(){
		nativeStop();
	}
	
	public int getDuration(){
		return nativeGetDuration();
	}
	
	public int getPosition(){
		return nativeGetPosition();
	}

	
	public void release(){
		nativeRelease();
	}
	
	static {
		/*
		System.loadLibrary("avutil-52");
		System.loadLibrary("avcodec-55");
		System.loadLibrary("avformat-55");
		System.loadLibrary("swresample-0");
		*/
		System.loadLibrary("aplayer-jni");
		nativeInit();
	}
	
	//native
	private int mNativeContext = 0;
	
	private static native final void nativeInit();
	private native final void nativeSetup(Object aplayer_this);
	private native final void nativeSetPath(String path);
	private native final void nativeSetSurface(Surface surfaace);
	private native final void nativePrepare();
	private native final void nativePlay();
	private native final void nativeSeek(int time);
	private native final void nativePause();
	private native final void nativeStop();
	private native final int nativeGetDuration();
	private native final int nativeGetPosition();
	private native final void nativeRelease();
	
	//native msg callback
	private static final int NATIVE_MSG_NOTIFY_VIDEO_SIZE = 0;
	private static final int NATIVE_MSG_NOTIFY_LOG_INFO = 1;
	private static final int NATIVE_MSG_NOTIFY_SET_LISTENER_DONE = 2;
	private static final int NATIVE_MSG_NOTIFY_SET_PATH_DONE = 3;
	private static final int NATIVE_MSG_NOTIFY_SET_VIDEO_SINK_DONE = 4;
	private static final int NATIVE_MSG_NOTIFY_PREPARE_DONE = 5;
	private static final int NATIVE_MSG_NOTIFY_PLAY_DONE = 6;
	private static final int NATIVE_MSG_NOTIFY_SEEK_DONE = 7;
	private static final int NATIVE_MSG_NOTIFY_PAUSE_DONE = 8;
	private static final int NATIVE_MSG_NOTIFY_ERROR = 9;

	
    private static void postEventFromNative(Object aplayer_ref, int what, int arg1, int arg2, byte[] bytes){
    	APlayer ap = (APlayer)((WeakReference)aplayer_ref).get();
    	if (ap == null) {
    		return;
    	}

    	if (ap.mEventHandler != null) {
    		Message m = ap.mEventHandler.obtainMessage(what, arg1, arg2);
    		m.obj = bytes;
    		ap.mEventHandler.sendMessage(m);
    	}
    }

}
