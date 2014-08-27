package com.openamedia.client;

import java.io.File;
import java.io.FilenameFilter;
import java.util.ArrayList;
import java.util.List;

import android.support.v7.app.ActionBarActivity;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnFocusChangeListener;
import android.widget.ArrayAdapter;
import android.widget.AutoCompleteTextView;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.Toast;
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;

public class MainActivity extends ActionBarActivity {
	private static final String TAG = "MainActivity";
	
	private static final String[] KNOWN_EXTENSIONS = new String[] {
		".mp4",
		".flv",
		".f4v",
		".mp3",
	};
	
	public static final String INTENT_KEY_MEDIA_PATH = "path";
	
	private ImageButton mPlayButton = null;
	private AutoCompleteTextView mText = null;
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        List<String> medias = new ArrayList<String>();
        boolean sdCardExist = Environment.getExternalStorageState().equals(android.os.Environment.MEDIA_MOUNTED);
        if(!sdCardExist){
        	Toast.makeText(this, "no sdcard found!!", 3).show();
        }else{
        	File[] files = Environment.getExternalStorageDirectory().listFiles(new FilenameFilter(){
				@Override
				public boolean accept(File arg0, String arg1) {
					for(String str:KNOWN_EXTENSIONS){
						if(arg1.endsWith(str)){
							return true;
						}
					}
					
					return false;
				}
        	});
        	
        	for(File file:files){
        		medias.add(file.getAbsolutePath());
        	}
        	
        	//medias.add("http://k.youku.com/player/getFlvPath/sid/540869042021212d739e1_01/st/flv/fileid/030002080053F60752C22B03BAF2B131BAD93E-904A-45AB-45A9-76A73BA3F8C3?K=0865c9ccb42673dd2411e992&hd=0&ymovie=1&myp=0&ts=367&ypp=2&ctype=12&ev=1&token=6794&oip=3663591661&ep=dyaUEk%2BEVsoC5SHeij8bMyvhfSIOXP4J9h%2BFg9JqALshS565mTnUxe6yTI1CFYtoBCB0Eejw2aGTY0NhYflDrRkQ20vZS%2Frk%2B4Hq5apVtZcDFGg1BMWkt1SYRDn1");//for http test
        	//medias.add("http://pl.youku.com/playlist/m3u8?ts=1408687433&keyframe=0&vid=XNTUzMDQzODky&type=flv&ep=cSaUEk%2BFX8wI5SbXiD8bNS%2BxdSZcXJZ0knrP%2FKYDSsRQE6HQnT%2FWzg%3D%3D&sid=340868928058012b3c1ac&token=4179&ctype=12&ev=1&oip=3663591661");//for HLS test.
        }
        
		ArrayAdapter<String> av = new ArrayAdapter<String>(this, R.layout.drop_down_item, medias.toArray(new String[0]));
		mText = (AutoCompleteTextView) findViewById(R.id.autoCompleteTextView1);
		mText.setAdapter(av);
		mText.setDropDownHeight(350);
		mText.setOnFocusChangeListener(new OnFocusChangeListener() {
            @Override  
            public void onFocusChange(View v, boolean hasFocus) {
                AutoCompleteTextView view = (AutoCompleteTextView) v;  
                if (hasFocus) {
                    view.showDropDown();  
                }
            }  
        }); 
		
		mPlayButton = (ImageButton)this.findViewById(R.id.imageButton1);
		mPlayButton.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View arg0) {
				Intent intent = new Intent();
				intent.putExtra(INTENT_KEY_MEDIA_PATH, mText.getText().toString());
				intent.setClass(MainActivity.this, PlayerActivity.class);
				MainActivity.this.startActivity(intent);
			}
			
		});
    }

    
}
