package com.mod;

import android.app.Activity;
import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.GradientDrawable;
import android.text.Editable;
import android.text.InputType;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.Locale;

/** Mod menu flutuante para Inotia 4 1.4.0: gold com valor livre e lista de itens com nome. Sem permissao de overlay. */
public class ModMenu {
    static { try { System.loadLibrary("modmenu"); } catch (Throwable t) { } }

    public static native boolean nativeReady();
    public static native boolean nativeAddMoney(long v);
    public static native long nativeGetMoney();
    public static native String[] nativeItemNames();
    public static native byte[] nativeItemFlags();
    public static native int nativeAddItem(int idx, int count);
    public static native boolean nativeSave();

    private static final int ORANGE = 0xFFFF7A1A;
    private static final int PANEL_BG = 0xF2161826;
    private static final int FIELD_BG = 0x33FFFFFF;

    private static boolean started = false;

    // estado da lista de itens
    private static String[] names;
    private static byte[] flags;
    private static final ArrayList<Integer> shown = new ArrayList<Integer>();
    private static int selected = -1;
    private static BaseAdapter adapter;
    private static TextView selLabel;
    private static TextView goldInfo;
    private static String filterText = "";
    private static ListView listView;
    private static boolean listBound = false;

    public static void start(final Activity a) {
        if (started) return;
        started = true;
        a.getWindow().getDecorView().postDelayed(new Runnable() {
            public void run() { try { build(a); } catch (Throwable t) { started = false; } }
        }, 4000);
    }

    private static int dp(Activity a, int v) { return (int) (v * a.getResources().getDisplayMetrics().density); }

    private static GradientDrawable round(int color, int radiusPx) {
        GradientDrawable g = new GradientDrawable();
        g.setColor(color);
        g.setCornerRadius(radiusPx);
        return g;
    }

    private static Button flat(Activity a, String t, int color, View.OnClickListener l) {
        Button b = new Button(a);
        b.setText(t);
        b.setTextSize(12);
        b.setAllCaps(false);
        b.setTextColor(Color.WHITE);
        b.setMinWidth(0); b.setMinimumWidth(0); b.setMinHeight(0); b.setMinimumHeight(0);
        b.setPadding(dp(a, 8), dp(a, 8), dp(a, 8), dp(a, 8));
        b.setBackground(round(color, dp(a, 8)));
        b.setOnClickListener(l);
        return b;
    }

    private static EditText field(Activity a, String hint, boolean number) {
        EditText e = new EditText(a);
        e.setHint(hint);
        e.setHintTextColor(0x88FFFFFF);
        e.setTextColor(Color.WHITE);
        e.setTextSize(13);
        e.setSingleLine(true);
        e.setPadding(dp(a, 8), dp(a, 6), dp(a, 8), dp(a, 6));
        e.setBackground(round(FIELD_BG, dp(a, 8)));
        if (number) e.setInputType(InputType.TYPE_CLASS_NUMBER);
        return e;
    }

    private static TextView label(Activity a, String t, int size, int color) {
        TextView v = new TextView(a);
        v.setText(t);
        v.setTextSize(size);
        v.setTextColor(color);
        return v;
    }

    private static LinearLayout.LayoutParams lpMatch(Activity a, int topDp) {
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        p.topMargin = dp(a, topDp);
        return p;
    }

    private static void hideKeyboard(Activity a, View v) {
        try {
            InputMethodManager imm = (InputMethodManager) a.getSystemService(Context.INPUT_METHOD_SERVICE);
            if (imm != null) imm.hideSoftInputFromWindow(v.getWindowToken(), 0);
        } catch (Throwable t) { }
    }

    private static void refreshGold() {
        if (goldInfo == null) return;
        goldInfo.setText(nativeReady() ? ("Gold atual: " + nativeGetMoney()) : "libgame.so nao encontrada / simbolos ausentes");
    }

    private static boolean itemsLoaded = false;

    private static void loadItems(Activity a) {
        if (itemsLoaded) return;
        try {
            names = nativeItemNames();
            flags = nativeItemFlags();
        } catch (Throwable t) { names = null; }
        if (names == null || flags == null) {
            Toast.makeText(a, "Lista de itens indisponivel (textos do jogo ainda nao carregados). Feche e abra o menu.", Toast.LENGTH_LONG).show();
            return;
        }
        itemsLoaded = true;
        applyFilter(filterText);
    }

    // chamado DEPOIS do painel ser desenhado: carrega os nomes e religa o adapter na primeira vez
    private static void showItems(Activity a) {
        loadItems(a);
        if (itemsLoaded && listView != null) {
            applyFilter(filterText);
            if (!listBound) { listView.setAdapter(adapter); listBound = true; }
            listView.invalidateViews();
        }
        refreshGold();
    }

    private static void applyFilter(String q) {
        filterText = q;
        shown.clear();
        if (names != null) {
            String f = q.trim().toLowerCase(Locale.getDefault());
            for (int i = 0; i < names.length; i++) {
                String n = names[i];
                if (n == null || n.length() == 0) continue;     // sem nome: nao aparece
                if (f.length() == 0 || n.toLowerCase(Locale.getDefault()).contains(f)) shown.add(i);
            }
        }
        if (adapter != null) adapter.notifyDataSetChanged();
    }

    private static void build(final Activity a) {
        DisplayMetrics dm = a.getResources().getDisplayMetrics();
        final int sw = dm.widthPixels, sh = dm.heightPixels;
        final int panelW = Math.min(sw - dp(a, 16), dp(a, 560));
        final int panelH = Math.min(sh - dp(a, 16), dp(a, 320));

        final FrameLayout root = new FrameLayout(a);

        // ---------- painel (2 colunas) ----------
        final LinearLayout panel = new LinearLayout(a);
        panel.setOrientation(LinearLayout.HORIZONTAL);
        panel.setBackground(round(PANEL_BG, dp(a, 14)));
        panel.setPadding(dp(a, 10), dp(a, 10), dp(a, 10), dp(a, 10));
        panel.setVisibility(View.GONE);

        // coluna esquerda: gold + item selecionado + quantidade
        LinearLayout left = new LinearLayout(a);
        left.setOrientation(LinearLayout.VERTICAL);
        ScrollView leftScroll = new ScrollView(a);
        leftScroll.addView(left);
        panel.addView(leftScroll, new LinearLayout.LayoutParams(dp(a, 190), ViewGroup.LayoutParams.MATCH_PARENT));

        TextView title = label(a, "MOD MENU", 15, ORANGE);
        title.setTypeface(null, android.graphics.Typeface.BOLD);
        left.addView(title);

        goldInfo = label(a, "", 12, Color.WHITE);
        left.addView(goldInfo, lpMatch(a, 6));

        final EditText goldInput = field(a, "Quantidade de gold", true);
        left.addView(goldInput, lpMatch(a, 4));
        left.addView(flat(a, "Adicionar gold", ORANGE, new View.OnClickListener() { public void onClick(View v) {
            hideKeyboard(a, goldInput);
            String s = goldInput.getText().toString().trim();
            long amount;
            try {
                if (s.length() == 0 || s.length() > 18) throw new NumberFormatException();
                amount = Long.parseLong(s);
            } catch (NumberFormatException ex) {
                Toast.makeText(a, "Digite um valor de gold valido", Toast.LENGTH_SHORT).show();
                return;
            }
            if (amount <= 0) { Toast.makeText(a, "O valor precisa ser maior que zero", Toast.LENGTH_SHORT).show(); return; }
            boolean ok = nativeAddMoney(amount);
            refreshGold();
            Toast.makeText(a, ok ? ("+" + amount + " gold") : "Nao foi possivel adicionar (limite de gold?)", Toast.LENGTH_SHORT).show();
        }}), lpMatch(a, 4));

        View sep = new View(a);
        sep.setBackgroundColor(0x33FFFFFF);
        LinearLayout.LayoutParams sp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(a, 1));
        sp.topMargin = dp(a, 10);
        left.addView(sep, sp);

        selLabel = label(a, "Item: (selecione na lista)", 12, Color.WHITE);
        left.addView(selLabel, lpMatch(a, 8));

        final EditText qtyInput = field(a, "Quantidade", true);
        qtyInput.setText("1");
        left.addView(qtyInput, lpMatch(a, 4));

        left.addView(flat(a, "Adicionar item", 0xFF2E9E5B, new View.OnClickListener() { public void onClick(View v) {
            hideKeyboard(a, qtyInput);
            if (selected < 0) { Toast.makeText(a, "Selecione um item na lista", Toast.LENGTH_SHORT).show(); return; }
            int qty;
            try { qty = Integer.parseInt(qtyInput.getText().toString().trim()); } catch (NumberFormatException ex) { qty = 0; }
            if (qty < 1 || qty > 9999) { Toast.makeText(a, "Quantidade entre 1 e 9999", Toast.LENGTH_SHORT).show(); return; }
            int got = nativeAddItem(selected, qty);
            Toast.makeText(a, got > 0 ? ("Entraram " + got + " de " + qty)
                : "Nenhum item entrou (mochila cheia?)", Toast.LENGTH_SHORT).show();
        }}), lpMatch(a, 6));

        left.addView(flat(a, "Salvar jogo", 0xFF3B6FD4, new View.OnClickListener() { public void onClick(View v) {
            Toast.makeText(a, nativeSave() ? "Jogo salvo" : "Falhou ao salvar", Toast.LENGTH_SHORT).show();
        }}), lpMatch(a, 6));

        // coluna direita: busca + lista
        LinearLayout right = new LinearLayout(a);
        right.setOrientation(LinearLayout.VERTICAL);
        LinearLayout.LayoutParams rp = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 1f);
        rp.leftMargin = dp(a, 10);
        panel.addView(right, rp);

        EditText search = field(a, "Buscar item pelo nome...", false);
        search.addTextChangedListener(new TextWatcher() {
            public void beforeTextChanged(CharSequence s, int st, int c, int af) { }
            public void onTextChanged(CharSequence s, int st, int b, int c) { }
            public void afterTextChanged(Editable s) { applyFilter(s.toString()); }
        });
        right.addView(search, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        final ListView list = new ListView(a);
        list.setDivider(null);
        list.setVerticalScrollBarEnabled(true);
        adapter = new BaseAdapter() {
            public int getCount() { return shown.size(); }
            public Object getItem(int p) { return shown.get(p); }
            public long getItemId(int p) { return shown.get(p); }
            public View getView(int p, View cv, ViewGroup parent) {
                TextView t = (cv instanceof TextView) ? (TextView) cv : new TextView(a);
                int idx = shown.get(p);
                boolean cash = flags != null && idx < flags.length && (flags[idx] & 2) != 0;
                t.setText(names[idx] + (cash ? "  [cash]" : ""));
                t.setTextSize(13);
                t.setTextColor(Color.WHITE);
                t.setSingleLine(true);
                t.setEllipsize(TextUtils.TruncateAt.END);
                t.setPadding(dp(a, 10), dp(a, 9), dp(a, 10), dp(a, 9));
                t.setBackground(idx == selected ? round(0x66FF7A1A, dp(a, 6)) : null);
                return t;
            }
        };
        list.setAdapter(adapter);
        listView = list;
        list.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View view, int pos, long id) {
                selected = shown.get(pos);
                selLabel.setText("Item: " + names[selected]);
                adapter.notifyDataSetChanged();
            }
        });
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f);
        lp.topMargin = dp(a, 6);
        right.addView(list, lp);

        FrameLayout.LayoutParams pp = new FrameLayout.LayoutParams(panelW, panelH);
        pp.leftMargin = dp(a, 70); pp.topMargin = dp(a, 12);
        root.addView(panel, pp);

        // ---------- botao flutuante ----------
        final Button fab = new Button(a);
        fab.setText("MOD");
        fab.setTextSize(11);
        fab.setTextColor(Color.WHITE);
        fab.setAllCaps(false);
        fab.setBackground(round(0xCCFF6600, dp(a, 28)));
        FrameLayout.LayoutParams fp = new FrameLayout.LayoutParams(dp(a, 56), dp(a, 56));
        fp.leftMargin = dp(a, 8); fp.topMargin = dp(a, 12);
        root.addView(fab, fp);

        final int slop = dp(a, 12); // tolerancia: ate 12dp de tremida ainda conta como toque
        fab.setOnTouchListener(new View.OnTouchListener() {
            float dx, dy, downX, downY; boolean moved;
            public boolean onTouch(View v, MotionEvent e) {
                FrameLayout.LayoutParams f = (FrameLayout.LayoutParams) v.getLayoutParams();
                switch (e.getActionMasked()) {
                    case MotionEvent.ACTION_DOWN:
                        downX = e.getRawX(); downY = e.getRawY();
                        dx = downX - f.leftMargin; dy = downY - f.topMargin; moved = false; return true;
                    case MotionEvent.ACTION_MOVE:
                        if (!moved && Math.abs(e.getRawX() - downX) < slop && Math.abs(e.getRawY() - downY) < slop) return true;
                        if (!moved) { dx = e.getRawX() - f.leftMargin; dy = e.getRawY() - f.topMargin; }
                        f.leftMargin = (int) (e.getRawX() - dx); f.topMargin = (int) (e.getRawY() - dy);
                        moved = true; v.setLayoutParams(f);
                        FrameLayout.LayoutParams q = (FrameLayout.LayoutParams) panel.getLayoutParams();
                        q.leftMargin = Math.max(0, Math.min(f.leftMargin + dp(a, 62), sw - panelW));
                        q.topMargin = Math.max(0, Math.min(f.topMargin, sh - panelH));
                        panel.setLayoutParams(q);
                        return true;
                    case MotionEvent.ACTION_UP:
                        if (!moved) {
                            try {
                                boolean open = panel.getVisibility() == View.GONE;
                                panel.setVisibility(open ? View.VISIBLE : View.GONE);
                                if (open) {
                                    refreshGold();
                                    panel.post(new Runnable() { public void run() {
                                        try { showItems(a); } catch (Throwable t2) {
                                            Toast.makeText(a, "Erro na lista: " + t2, Toast.LENGTH_LONG).show();
                                        }
                                    }});
                                } else hideKeyboard(a, v);
                            } catch (Throwable t) {
                                Toast.makeText(a, "Erro no menu: " + t, Toast.LENGTH_LONG).show();
                            }
                        }
                        return true;
                    case MotionEvent.ACTION_CANCEL:
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
