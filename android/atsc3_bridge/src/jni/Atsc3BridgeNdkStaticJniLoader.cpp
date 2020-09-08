//
// Created by Jason Justman on 8/18/20.
//

#include "Atsc3BridgeNdkStaticJniLoader.h"

JavaVM* javaVM_scoped = nullptr;

JavaVM* atsc3_bridge_ndk_static_loader_get_javaVM() {
    return javaVM_scoped;

}
extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    javaVM_scoped = vm;

    _BRIDGE_NDK_STATIC_JNI_LOADER_INFO("atsc3_bridge::JNI_OnLoad complete, vm: %p", vm);
    return JNI_VERSION_1_6;
}


