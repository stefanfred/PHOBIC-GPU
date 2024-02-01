#include "renderdoc.h"

#include <cassert>

#ifdef ENABLE_RENDERDOC
    #include "renderdoc_app.h"

    #ifdef _WIN32
        #include <windows.h>
    #elif __linux__
        #include <dlfcn.h>
    #endif

    static RENDERDOC_API_1_1_2 *rdoc_api = NULL;
#endif

namespace renderdoc {
    void initialize() {
        #ifdef ENABLE_RENDERDOC
            pRENDERDOC_GetAPI RENDERDOC_GetAPI = nullptr;

            #ifdef _WIN32
                if(HMODULE mod = GetModuleHandleA("renderdoc.dll"))
                    RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
            #elif __linux__
                if(void *mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD))
                    RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
            #endif

            if (RENDERDOC_GetAPI != nullptr) {
                int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void **)&rdoc_api);
                assert(ret == 1);
            }
        #endif
    }

    void startCapture() {
        #ifdef ENABLE_RENDERDOC
            if (rdoc_api)
                rdoc_api->StartFrameCapture(NULL, NULL);
        #endif
    }

    void endCapture() {
        #ifdef ENABLE_RENDERDOC
            if (rdoc_api)
                rdoc_api->EndFrameCapture(NULL, NULL);
        #endif
    }
}
