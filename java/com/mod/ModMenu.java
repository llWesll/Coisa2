package com.mod;

import android.app.Activity;
import android.graphics.Color;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

/** Float mod menu para Inotia 4 1.4.0. Sem permissao de overlay: usa addContentView da Activity. */
public class ModMenu {
    static { try { System.loadLibrary("modmenu"); } catch (Throwable t) { } }

    public static native boolean nativeReady();
    public static native long nativeGetGems();
    public static native void nativeSetGems(long v);
    public static native boolean nativeAddMoney(long v);
    public static native long nativeGetMoney();
    public static native String nativeDump();

    private static boolean started = false;

    public static void start(final Activity a) {
        if (started) return;
        started = true;
        a.getWindow().getDecorView().postDelayed(new Runnable() {
            public void run() { try { build(a); } catch (Throwable t) { started = false; } }
        }, 4000);
    }

    private static int dp(Activity a, int v) { return (int) (v * a.getResources().getDisplayMetrics().density); }

    private static Button btn(Activity a, String t, View.OnClickListener l) {
        Button b = new Button(a); b.setText(t); b.setTextSize(12); b.setOnClickListener(l); return b;
    }

    private static void build(final Activity a) {
        final FrameLayout root = new FrameLayout(a);

        final LinearLayout panel = new LinearLayout(a);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setBackgroundColor(0xE6202030);
        panel.setPadding(dp(a, 10), dp(a, 10), dp(a, 10), dp(a, 10));
        panel.setVisibility(View.GONE);

        final TextView info = new TextView(a);
        info.setTextColor(Color.WHITE);
        info.setTextSize(13);
        panel.addView(info);

        final Runnable refresh = new Runnable() { public void run() {
            info.setText(nativeReady()
                ? ("Gems: " + nativeGetGems() + "   Gold: " + nativeGetMoney())
                : "libgame.so nao encontrada / simbolos ausentes");
        }};

        panel.addView(btn(a, "+100 Gems", new View.OnClickListener() { public void onClick(View v) {
            nativeSetGems(nativeGetGems() + 100); refresh.run(); }}));
        panel.addView(btn(a, "+1000 Gems", new View.OnClickListener() { public void onClick(View v) {
            nativeSetGems(nativeGetGems() + 1000); refresh.run(); }}));
        panel.addView(btn(a, "+10000 Gems", new View.OnClickListener() { public void onClick(View v) {
            nativeSetGems(nativeGetGems() + 10000); refresh.run(); }}));
        panel.addView(btn(a, "+100000 Gold", new View.OnClickListener() { public void onClick(View v) {
            nativeAddMoney(100000); refresh.run(); }}));

        panel.addView(btn(a, "+1000000 Gold", new View.OnClickListener() { public void onClick(View v) {
            nativeAddMoney(1000000); refresh.run(); }}));
        panel.addView(btn(a, "Salvar dump (diagnostico)", new View.OnClickListener() { public void onClick(View v) {
            try {
                java.io.File dir = a.getExternalFilesDir(null);
                java.io.File f = new java.io.File(dir, "modmenu_dump.txt");
                java.io.FileOutputStream o = new java.io.FileOutputStream(f);
                o.write(nativeDump().getBytes("UTF-8")); o.close();
                Toast.makeText(a, "Salvo em " + f.getAbsolutePath(), Toast.LENGTH_LONG).show();
            } catch (Throwable t) { Toast.makeText(a, "Falhou: " + t, Toast.LENGTH_LONG).show(); }
        }}));

        FrameLayout.LayoutParams pp = new FrameLayout.LayoutParams(dp(a, 230), ViewGroup.LayoutParams.WRAP_CONTENT);
        pp.leftMargin = dp(a, 70); pp.topMargin = dp(a, 20);
        root.addView(panel, pp);

        final Button fab = new Button(a);
        fab.setText("MOD");
        fab.setTextSize(11);
        fab.setBackgroundColor(0xCCFF6600);
        fab.setTextColor(Color.WHITE);
        FrameLayout.LayoutParams fp = new FrameLayout.LayoutParams(dp(a, 56), dp(a, 56));
        fp.leftMargin = dp(a, 8); fp.topMargin = dp(a, 20);
        root.addView(fab, fp);

        fab.setOnTouchListener(new View.OnTouchListener() {
            float dx, dy; boolean moved;
            public boolean onTouch(View v, MotionEvent e) {
                FrameLayout.LayoutParams lp = (FrameLayout.LayoutParams) v.getLayoutParams();
                switch (e.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        dx = e.getRawX() - lp.leftMargin; dy = e.getRawY() - lp.topMargin; moved = false; return true;
                    case MotionEvent.ACTION_MOVE:
                        lp.leftMargin = (int) (e.getRawX() - dx); lp.topMargin = (int) (e.getRawY() - dy);
                        moved = true; v.setLayoutParams(lp);
                        FrameLayout.LayoutParams q = (FrameLayout.LayoutParams) panel.getLayoutParams();
                        q.leftMargin = lp.leftMargin + dp(a, 62); q.topMargin = lp.topMargin; panel.setLayoutParams(q);
                        return true;
                    case MotionEvent.ACTION_UP:
                        if (!moved) {
                            panel.setVisibility(panel.getVisibility() == View.GONE ? View.VISIBLE : View.GONE);
                            refresh.run();
                        }
                        return true;
                }
                return false;
            }
        });

        a.addContentView(root, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        root.setClickable(false);
        root.setFocusable(false);
    }
}
