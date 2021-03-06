// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#include "glass/support/IniSaverBase.h"

#include <imgui_internal.h>

using namespace glass;

namespace {
class ImGuiSaver : public IniSaverBackend {
 public:
  void Register(IniSaverBase* iniSaver) override;
  void Unregister(IniSaverBase* iniSaver) override;
};
}  // namespace

void ImGuiSaver::Register(IniSaverBase* iniSaver) {
  // hook ini handler to save settings
  ImGuiSettingsHandler iniHandler;
  iniHandler.TypeName = iniSaver->GetTypeName();
  iniHandler.TypeHash = ImHashStr(iniHandler.TypeName);
  iniHandler.ReadOpenFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler,
                             const char* name) {
    return static_cast<IniSaverBase*>(handler->UserData)->IniReadOpen(name);
  };
  iniHandler.ReadLineFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler,
                             void* entry, const char* line) {
    static_cast<IniSaverBase*>(handler->UserData)->IniReadLine(entry, line);
  };
  iniHandler.WriteAllFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler,
                             ImGuiTextBuffer* out_buf) {
    static_cast<IniSaverBase*>(handler->UserData)->IniWriteAll(out_buf);
  };
  iniHandler.UserData = iniSaver;
  ImGui::GetCurrentContext()->SettingsHandlers.push_back(iniHandler);
}

void ImGuiSaver::Unregister(IniSaverBase* iniSaver) {
  if (auto ctx = ImGui::GetCurrentContext()) {
    auto& handlers = ctx->SettingsHandlers;
    for (auto it = handlers.begin(), end = handlers.end(); it != end; ++it) {
      if (it->UserData == iniSaver) {
        handlers.erase(it);
        return;
      }
    }
  }
}

static ImGuiSaver* GetSaverInstance() {
  static ImGuiSaver* inst = new ImGuiSaver;
  return inst;
}

IniSaverBase::IniSaverBase(std::string_view typeName, IniSaverBackend* backend)
    : m_typeName(typeName), m_backend{backend ? backend : GetSaverInstance()} {}

IniSaverBase::~IniSaverBase() {
  m_backend->Unregister(this);
}
