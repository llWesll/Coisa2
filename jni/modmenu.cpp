// libmodmenu.so - resolve simbolos EXPORTADOS de libgame.so (Inotia 4 1.4.0) via dlsym.
#include <jni.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <android/log.h>

static void* H() {
    static void* h = nullptr;
    if (!h) h = dlopen("libgame.so", RTLD_NOW | RTLD_NOLOAD);
    return h;
}
template <class T> static T sym(const char* n) { void* h = H(); return h ? (T)dlsym(h, n) : nullptr; }

// ---- itens: replica NetworkStore_AddItem (CreateItem -> qtd nos bits 31..25 de ITEM+0x10 -> INVEN_SaveItem)
// ITEM tem 40 bytes; +0x08 u16 bits 15..6 = indice no ITEMDATABASE; ITEMDATABASE rec[6] bit0 = empilhavel (max 99).
// ITEMDATABASE rec[0] (u16) = id do texto do nome (MEMORYTEXT_GetText); rec[2] = classe (ITEMCLASSBASE).
static const uint8_t* itemRec(int idx) {
    auto pd = sym<uint8_t**>("ITEMDATABASE_pData");
    auto sz = sym<uint8_t*>("ITEMDATABASE_nRecordSize");
    auto n = sym<uint16_t*>("ITEMDATABASE_nRecordCount");
    if (!pd || !*pd || !sz || !n || idx < 0 || idx >= (int)*n) return nullptr;
    return *pd + (size_t)(*sz) * (size_t)idx;
}

static bool stackable(int idx) {
    const uint8_t* r = itemRec(idx);
    return r && (r[6] & 1);
}

// classe com bit 3 = item cash (vai para a bolsa 5)
static bool isCash(int idx) {
    const uint8_t* r = itemRec(idx);
    auto pd = sym<uint8_t**>("ITEMCLASSBASE_pData");
    auto sz = sym<uint8_t*>("ITEMCLASSBASE_nRecordSize");
    auto n = sym<uint16_t*>("ITEMCLASSBASE_nRecordCount");
    if (!r || !pd || !*pd || !sz || !n || r[2] >= *n) return false;
    return ((*pd)[(size_t)(*sz) * r[2] + 2] & 8) != 0;
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

// Copia o texto do jogo para UTF-8 valido (ate 3 bytes por caractere), pois NewStringUTF aborta com UTF-8 invalido.
static std::string cleanUtf8(const char* s) {
    std::string out;
    if (!s) return out;
    size_t n = strnlen(s, 96);
    for (size_t i = 0; i < n;) {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x20) { out += ' '; i++; }
        else if (c < 0x80) { out += (char)c; i++; }
        else if (c >= 0xC2 && c <= 0xDF && i + 1 < n && ((unsigned char)s[i + 1] & 0xC0) == 0x80) { out.append(s + i, 2); i += 2; }
        else if (c >= 0xE0 && c <= 0xEF && i + 2 < n && ((unsigned char)s[i + 1] & 0xC0) == 0x80 &&
                 ((unsigned char)s[i + 2] & 0xC0) == 0x80) { out.append(s + i, 3); i += 3; }
        else { out += '?'; i++; }
    }
    return out;
}

extern "C" {

JNIEXPORT jboolean JNICALL Java_com_mod_ModMenu_nativeReady(JNIEnv*, jclass) {
    return sym<void*>("INVEN_AddMoney") != nullptr && sym<void*>("ITEMSYSTEM_CreateItem") != nullptr;
}

// INVEN_AddMoney(uint64 qtd) -> bool (assinatura confirmada no disassembly). false = estouro.
JNIEXPORT jboolean JNICALL Java_com_mod_ModMenu_nativeAddMoney(JNIEnv*, jclass, jlong v) {
    auto f = sym<int (*)(uint64_t)>("INVEN_AddMoney");
    return (f && v > 0) ? (jboolean)(f((uint64_t)v) & 1) : JNI_FALSE;
}

JNIEXPORT jlong JNICALL Java_com_mod_ModMenu_nativeGetMoney(JNIEnv*, jclass) {
    auto f = sym<int64_t (*)()>("INVEN_GetMoney");
    return f ? (jlong)f() : -1;
}

// Nomes de todos os itens (posicao = indice no ITEMDATABASE). "" = sem nome / texto indisponivel.
JNIEXPORT jobjectArray JNICALL Java_com_mod_ModMenu_nativeItemNames(JNIEnv* env, jclass) {
    auto cnt = sym<uint16_t*>("ITEMDATABASE_nRecordCount");
    auto getText = sym<const char* (*)(int)>("MEMORYTEXT_GetText");
    auto tp = sym<void**>("MEMORYTEXT_pData");
    auto tn = sym<uint16_t*>("MEMORYTEXT_nRecordCount");
    if (!cnt || !getText || !tp || !*tp || !tn || *tn == 0) return nullptr;
    jclass sc = env->FindClass("java/lang/String");
    jobjectArray arr = env->NewObjectArray((jsize)*cnt, sc, nullptr);
    for (int i = 0; i < (int)*cnt; i++) {
        const uint8_t* r = itemRec(i);
        std::string nm;
        if (r) nm = cleanUtf8(getText((int)(r[0] | (r[1] << 8))));
        jstring js = env->NewStringUTF(nm.c_str());
        env->SetObjectArrayElement(arr, i, js);
        env->DeleteLocalRef(js);
    }
    return arr;
}

// flags por item: bit0 = empilhavel, bit1 = cash
JNIEXPORT jbyteArray JNICALL Java_com_mod_ModMenu_nativeItemFlags(JNIEnv* env, jclass) {
    auto cnt = sym<uint16_t*>("ITEMDATABASE_nRecordCount");
    if (!cnt) return nullptr;
    int n = *cnt;
    jbyteArray arr = env->NewByteArray(n);
    jbyte* buf = new jbyte[n];
    for (int i = 0; i < n; i++) buf[i] = (jbyte)((stackable(i) ? 1 : 0) | (isCash(i) ? 2 : 0));
    env->SetByteArrayRegion(arr, 0, n, buf);
    delete[] buf;
    return arr;
}

// adiciona 'count' unidades do item de indice 'idx'. devolve quantas entraram.
JNIEXPORT jint JNICALL Java_com_mod_ModMenu_nativeAddItem(JNIEnv*, jclass, jint idx, jint count) {
    if (!itemRec(idx) || count < 1) return 0;
    int done = 0;
    if (stackable(idx)) {
        while (done < count) {
            int c = count - done > 99 ? 99 : count - done;
            if (!addOne(idx, c)) break;
            done += c;
        }
    } else {
        while (done < count && done < 99) { if (!addOne(idx, 1)) break; done++; }
    }
    __android_log_print(ANDROID_LOG_INFO, "ModMenu", "AddItem idx=%d pedido=%d entrou=%d", idx, count, done);
    return done;
}

// SAVE_Save(): o jogo chama isso logo depois de uma compra
JNIEXPORT jboolean JNICALL Java_com_mod_ModMenu_nativeSave(JNIEnv*, jclass) {
    auto f = sym<bool (*)()>("SAVE_Save");
    return f ? (jboolean)f() : JNI_FALSE;
}

} // extern "C"
