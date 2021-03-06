// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#pragma once

#include <string_view>

#include <imgui.h>

namespace glass {

class NameInfo {
 public:
  NameInfo() { m_name[0] = '\0'; }

  bool HasName() const { return m_name[0] != '\0'; }
  void SetName(std::string_view name);
  const char* GetName() const { return m_name; }
  void GetName(char* buf, size_t size, const char* defaultName) const;
  void GetName(char* buf, size_t size, const char* defaultName,
               int index) const;
  void GetName(char* buf, size_t size, const char* defaultName, int index,
               int index2) const;
  void GetLabel(char* buf, size_t size, const char* defaultName) const;
  void GetLabel(char* buf, size_t size, const char* defaultName,
                int index) const;
  void GetLabel(char* buf, size_t size, const char* defaultName, int index,
                int index2) const;

  bool ReadIni(std::string_view name, std::string_view value);
  void WriteIni(ImGuiTextBuffer* out);
  void PushEditNameId(int index);
  void PushEditNameId(const char* name);
  bool PopupEditName(int index);
  bool PopupEditName(const char* name);
  bool InputTextName(const char* label_id, ImGuiInputTextFlags flags = 0);

 private:
  char m_name[64];
};

class OpenInfo {
 public:
  OpenInfo() = default;
  explicit OpenInfo(bool open) : m_open(open) {}

  bool IsOpen() const { return m_open; }
  void SetOpen(bool open) { m_open = open; }
  bool ReadIni(std::string_view name, std::string_view value);
  void WriteIni(ImGuiTextBuffer* out);

 private:
  bool m_open = false;
};

class NameOpenInfo : public NameInfo, public OpenInfo {
 public:
  bool ReadIni(std::string_view name, std::string_view value);
  void WriteIni(ImGuiTextBuffer* out);
};

}  // namespace glass
