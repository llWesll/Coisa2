// libmodmenu.so - resolve simbolos EXPORTADOS de libgame.so (Inotia 4 1.4.0) via dlsym.
#include <jni.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <android/log.h>

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

// ---- itens cash: replica NetworkStore_AddItem (CreateItem -> qtd nos bits 31..25 de ITEM+0x10 -> INVEN_SaveItem)
// ITEM tem 40 bytes; +0x08 u16 bits 15..6 = indice no ITEMDATABASE; ITEMDATABASE rec[6] bit0 = empilhavel (max 99).
static bool stackable(int idx) {
    auto pd = sym<uint8_t**>("ITEMDATABASE_pData");
    auto sz = sym<uint8_t*>("ITEMDATABASE_nRecordSize");
    if (!pd || !*pd || !sz) return false;
    return ((*pd)[(size_t)(*sz) * (size_t)idx + 6] & 1) != 0;
}

static bool addOne(int idx, int cnt) {
    auto create = sym<void* (*)(int)>("ITEMSYSTEM_CreateItem");
    auto saveItem = sym<bool (*)(void*)>("INVEN_SaveItem");
    auto freeItem = sym<void (*)(void*)>("ITEMPOOL_Free");
    if (!create || !saveItem || !freeItem) return false;
    uint8_t* item = (uint8_t*)create(idx);
    if (!item) return false;
    if (stackable(idx)) {
        uint32_t v; memcpy(&v, item + 0x10, 4);
        v = (v & ~(0x7Fu << 25)) | ((uint32_t)(cnt & 0x7F) << 25);
        int64_t w = (int32_t)v;
        memcpy(item + 0x10, &w, 8);
    }
    if (!saveItem(item)) { freeItem(item); return false; } // mochila cheia
    return true; // se juntou numa pilha existente o proprio jogo libera 'item'
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

// INVEN_AddMoney(uint64 qtd) -> bool (assinatura confirmada no disassembly).
JNIEXPORT jboolean JNICALL Java_com_mod_ModMenu_nativeAddMoney(JNIEnv*, jclass, jlong v) {
    auto f = sym<int (*)(uint64_t)>("INVEN_AddMoney");
    return f ? (jboolean)(f((uint64_t)v) & 1) : JNI_FALSE;
}

JNIEXPORT jlong JNICALL Java_com_mod_ModMenu_nativeGetMoney(JNIEnv*, jclass) {
    auto f = sym<int64_t (*)()>("INVEN_GetMoney");
    return f ? (jlong)f() : -1;
}

JNIEXPORT jint JNICALL Java_com_mod_ModMenu_nativeCashCount(JNIEnv*, jclass) {
    auto c = sym<uint16_t*>("CASHITEMBASE_nRecordCount");
    return c ? (jint)*c : -1;
}

// entrada i do catalogo cash: indice | qtd<<16 | preco<<24 | grupo<<40 (layout deduzido do dump)
JNIEXPORT jlong JNICALL Java_com_mod_ModMenu_nativeCashEntry(JNIEnv*, jclass, jint i) {
    auto c = sym<uint16_t*>("CASHITEMBASE_nRecordCount");
    auto sz = sym<uint8_t*>("CASHITEMBASE_nRecordSize");
    auto pd = sym<uint8_t**>("CASHITEMBASE_pData");
    if (!c || !sz || !pd || !*pd || *sz < 8 || i < 0 || i >= (jint)*c) return -1;
    const uint8_t* r = *pd + (size_t)(*sz) * (size_t)i;
    jlong id = r[0] | (r[1] << 8), price = r[3] | (r[4] << 8);
    return id | ((jlong)r[2] << 16) | (price << 24) | ((jlong)r[7] << 40);
}

// devolve quantas unidades entraram na mochila
JNIEXPORT jint JNICALL Java_com_mod_ModMenu_nativeAddItem(JNIEnv*, jclass, jint idx, jint count) {
    auto n = sym<uint16_t*>("ITEMDATABASE_nRecordCount");
    if (!n || idx < 0 || idx >= (jint)*n || count < 1) return 0;
    int done = 0;
    if (stackable(idx)) {
        while (done < count) {
            int c = count - done > 99 ? 99 : count - done;
            if (!addOne(idx, c)) break;
            done += c;
        }
    } else {
        while (done < count && done < 20) { if (!addOne(idx, 1)) break; done++; }
    }
    __android_log_print(ANDROID_LOG_INFO, "ModMenu", "AddItem idx=%d pedido=%d entrou=%d", idx, count, done);
    return done;
}

// SAVE_Save(): o jogo chama isso logo depois de uma compra
JNIEXPORT jboolean JNICALL Java_com_mod_ModMenu_nativeSave(JNIEnv*, jclass) {
    auto f = sym<bool (*)()>("SAVE_Save");
    return f ? (jboolean)f() : JNI_FALSE;
}

// Diagnostico: despeja tabelas de itens e o inventario atual em texto, para descobrir
// o formato do item e montar o "adicionar item cash" com seguranca na proxima rodada.
JNIEXPORT jstring JNICALL Java_com_mod_ModMenu_nativeDump(JNIEnv* env, jclass) {
    std::string out;
    char b[160];
    out += "# Inotia4 dump\n";
    dumpTable(out, "CASHITEMBASE", 64);
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
