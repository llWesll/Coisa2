// libmodmenu.so - resolve simbolos EXPORTADOS de libgame.so (Inotia 4 1.4.0) via dlsym.
#include <jni.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <string>

static void* H() {
    static void* h = nullptr;
    if (!h) h = dlopen("libgame.so", RTLD_NOW | RTLD_NOLOAD);
    return h;
}
template <class T> static T sym(const char* n) { void* h = H(); return h ? (T)dlsym(h, n) : nullptr; }

static void hexdump(std::string& out, const char* label, const void* p, size_t n) {
    char b[16];
    out += label; out += ":\n";
    if (!p) { out += "  (null)\n"; return; }
    const uint8_t* q = (const uint8_t*)p;
    for (size_t i = 0; i < n; i++) {
        snprintf(b, sizeof b, "%02x", q[i]);
        out += b;
        out += ((i + 1) % 32 == 0) ? "\n" : " ";
    }
    out += "\n";
}

// Tabelas do jogo: <NOME>_nRecordCount (uint16), <NOME>_nRecordSize (uint8), <NOME>_pData (ponteiro)
static void dumpTable(std::string& out, const char* name, int maxRec) {
    std::string a = std::string(name) + "_nRecordCount";
    std::string b = std::string(name) + "_nRecordSize";
    std::string c = std::string(name) + "_pData";
    auto cnt = sym<uint16_t*>(a.c_str());
    auto sz = sym<uint8_t*>(b.c_str());
    auto pd = sym<uint8_t**>(c.c_str());
    char buf[128];
    snprintf(buf, sizeof buf, "== %s count=%d size=%d\n", name, cnt ? *cnt : -1, sz ? *sz : -1);
    out += buf;
    if (!cnt || !sz || !pd || !*pd || *sz == 0) return;
    int n = *cnt < maxRec ? *cnt : maxRec;
    for (int i = 0; i < n; i++) {
        snprintf(buf, sizeof buf, "[%d]", i);
        hexdump(out, buf, *pd + (size_t)i * *sz, *sz);
    }
}

extern "C" {

JNIEXPORT jboolean JNICALL Java_com_mod_ModMenu_nativeReady(JNIEnv*, jclass) {
    return sym<void*>("g_gemCount") != nullptr;
}

JNIEXPORT jlong JNICALL Java_com_mod_ModMenu_nativeGetGems(JNIEnv*, jclass) {
    auto p = sym<int64_t*>("g_gemCount");
    return p ? (jlong)*p : -1;
}

JNIEXPORT void JNICALL Java_com_mod_ModMenu_nativeSetGems(JNIEnv*, jclass, jlong v) {
    if (v < 0) v = 0;
    if (v > 999999999) v = 999999999;
    if (auto p = sym<int64_t*>("g_gemCount")) *p = v;
    if (auto q = sym<int64_t*>("NetworkStore_i32Gem")) *q = v; // espelho usado pela loja
}

// INVEN_AddMoney(uint64 qtd) -> bool (assinatura inferida do disassembly, nao confirmada).
JNIEXPORT jboolean JNICALL Java_com_mod_ModMenu_nativeAddMoney(JNIEnv*, jclass, jlong v) {
    auto f = sym<int (*)(uint64_t)>("INVEN_AddMoney");
    return f ? (jboolean)(f((uint64_t)v) & 1) : JNI_FALSE;
}

JNIEXPORT jlong JNICALL Java_com_mod_ModMenu_nativeGetMoney(JNIEnv*, jclass) {
    auto f = sym<int64_t (*)()>("INVEN_GetMoney");
    return f ? (jlong)f() : -1;
}

// Diagnostico: despeja tabelas de itens e o inventario atual em texto, para descobrir
// o formato do item e montar o "adicionar item cash" com seguranca na proxima rodada.
JNIEXPORT jstring JNICALL Java_com_mod_ModMenu_nativeDump(JNIEnv* env, jclass) {
    std::string out;
    char b[160];
    out += "# Inotia4 dump\n";
    dumpTable(out, "CASHITEMBASE", 40);
    dumpTable(out, "CASHITEMGROUPBASE", 12);
    dumpTable(out, "ITEMDATABASE", 12);
    hexdump(out, "INVEN_pItem (768 bytes)", sym<void*>("INVEN_pItem"), 768);
    hexdump(out, "INVEN_pBagSlot (48 bytes)", sym<void*>("INVEN_pBagSlot"), 48);
    const char* vars[] = {"NetworkStore_i32BuyItemIndex", "NetworkStore_i32BuyItemType",
                          "NetworkStore_i32UIInAppItemID", "NetworkStore_i32UIInAppItemPrice",
                          "NetworkStore_i32BuyProductIndex", "NetworkStore_i32Gem", "g_gemCount"};
    for (auto v : vars) {
        auto p = sym<int64_t*>(v);
        snprintf(b, sizeof b, "%s = %lld\n", v, p ? (long long)*p : -1LL);
        out += b;
    }
    return env->NewStringUTF(out.c_str());
}

} // extern "C"
