package {{bundle_id}};

import android.os.Bundle;
import android.app.Activity;
import android.net.Uri;
import android.content.Intent;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

private class MyWebViewClient extends WebViewClient {
  private Activity activity = null;

  public WebViewClientImpl(Activity activity) {
    this.activity = activity;
  }

  @Override
  public boolean shouldOverrideUrlLoading(WebView webView, String url) {
    if(url.indexOf("file://") > -1 ) return false;

    Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
    activity.startActivity(intent);

    return true;
  }
}

public class MainActivity extends Activity {
  private WebView webView = null;

	@Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
  
    this.webView = (WebView) findViewById(R.id.webview);

		WebSettings webSettings = webView.getSettings();
    webSettings.setJavaScriptEnabled(true);

    WebViewClientImpl webViewClient = new WebViewClientImpl(this);
    webView.setWebViewClient(webViewClient);

		webView.loadURL("file:///android_asset/index.html");
  }

  public native static fsOpen()
}
