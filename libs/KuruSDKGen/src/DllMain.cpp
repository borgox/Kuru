#include <windows.h>

#include <filesystem>
#include <string>
#include "TemplateBundle.hpp"

#include <Kuru/Common/DumpSchema.hpp>

namespace Kuru::Dump {
bool Run(void* selfModule, Kuru::Common::DumpSchema::DumpDocument& outDocument, std::string& outDir);
}
namespace Kuru::SdkGen {
bool Generate(const Kuru::Common::DumpSchema::DumpDocument& document, const std::filesystem::path& outputPath);
}

namespace {

HMODULE gSelf = nullptr;

DWORD WINAPI Worker(LPVOID) {
    Kuru::Common::DumpSchema::DumpDocument document;
    std::string dir;
    if (!Kuru::Dump::Run(gSelf, document, dir)) {
        MessageBoxA(nullptr, "Reflection walk failed, see KuruSDKGen.log.", "KuruSDKGen",
                    MB_ICONERROR | MB_OK);
        return 0;
    }

    const std::filesystem::path outputPath = std::filesystem::path(dir) / "generated-sdk";
    const bool ok = Kuru::SdkGen::Generate(document, outputPath);
    std::string message =
        ok ? "SDK generated.\n\nOutput:\n" + outputPath.string()
           : "SDK generation failed.\n\nSee KuruSDKGen.log in:\n" + dir;
    bool packaged = false;
    if (ok) {
        try {
            const auto modTemplate = Kuru::SdkGen::PackageTemplate(outputPath);
            message += "\n\nReady-to-build mod template:\n" + modTemplate.string();
            packaged = true;
        } catch (const std::exception& error) {
            message += "\n\nMod template packaging failed:\n" + std::string(error.what());
        }
    }
    MessageBoxA(nullptr, message.c_str(), "KuruSDKGen",
                (ok && packaged ? MB_ICONINFORMATION : MB_ICONERROR) | MB_OK);
    return 0;
}

}  // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
    (void)reserved;
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        gSelf = module;
        DisableThreadLibraryCalls(module);
        CreateThread(nullptr, 0, Worker, nullptr, 0, nullptr);
        break;
    default:
        break;
    }
    return TRUE;
}
