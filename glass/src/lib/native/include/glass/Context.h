// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <imgui.h>

namespace glass {

struct Context;

Context* CreateContext();
void DestroyContext(Context* ctx = nullptr);
Context* GetCurrentContext();
void SetCurrentContext(Context* ctx);

/**
 * Resets zero time to current time.
 */
void ResetTime();

/**
 * Gets the zero time.
 */
uint64_t GetZeroTime();

/**
 * Storage provides both persistent and non-persistent key/value storage for
 * widgets.
 *
 * Keys are always strings.  The storage also provides non-persistent arbitrary
 * data storage (via std::shared_ptr<void>).
 *
 * Storage is automatically indexed internally by the ID stack.  Note it is
 * necessary to use the glass wrappers for PushID et al to preserve naming in
 * the save file (unnamed values are still stored, but this is non-ideal for
 * users trying to hand-edit the save file).
 */
class Storage {
 public:
  struct Value {
    Value() = default;
    explicit Value(std::string_view str) : stringVal{str} {}

    enum Type { kNone, kInt, kInt64, kBool, kFloat, kDouble, kString };
    Type type = kNone;
    union {
      int intVal;
      int64_t int64Val;
      bool boolVal;
      float floatVal;
      double doubleVal;
    };
    std::string stringVal;
  };

  int GetInt(std::string_view key, int defaultVal = 0) const;
  int64_t GetInt64(std::string_view key, int64_t defaultVal = 0) const;
  bool GetBool(std::string_view key, bool defaultVal = false) const;
  float GetFloat(std::string_view key, float defaultVal = 0.0f) const;
  double GetDouble(std::string_view key, double defaultVal = 0.0) const;
  std::string GetString(std::string_view key,
                        std::string_view defaultVal = {}) const;

  void SetInt(std::string_view key, int val);
  void SetInt64(std::string_view key, int64_t val);
  void SetBool(std::string_view key, bool val);
  void SetFloat(std::string_view key, float val);
  void SetDouble(std::string_view key, double val);
  void SetString(std::string_view key, std::string_view val);

  int* GetIntRef(std::string_view key, int defaultVal = 0);
  int64_t* GetInt64Ref(std::string_view key, int64_t defaultVal = 0);
  bool* GetBoolRef(std::string_view key, bool defaultVal = false);
  float* GetFloatRef(std::string_view key, float defaultVal = 0.0f);
  double* GetDoubleRef(std::string_view key, double defaultVal = 0.0);
  std::string* GetStringRef(std::string_view key,
                            std::string_view defaultVal = {});

  Value& GetValue(std::string_view key);

  void SetData(std::shared_ptr<void>&& data) { m_data = std::move(data); }

  template <typename T>
  T* GetData() const {
    return static_cast<T*>(m_data.get());
  }

  Storage() = default;
  Storage(const Storage&) = delete;
  Storage& operator=(const Storage&) = delete;

  std::vector<std::string>& GetKeys() { return m_keys; }
  const std::vector<std::string>& GetKeys() const { return m_keys; }
  std::vector<std::unique_ptr<Value>>& GetValues() { return m_values; }
  const std::vector<std::unique_ptr<Value>>& GetValues() const {
    return m_values;
  }

 private:
  mutable std::vector<std::string> m_keys;
  mutable std::vector<std::unique_ptr<Value>> m_values;
  std::shared_ptr<void> m_data;
};

Storage& GetStorage();
Storage& GetStorage(std::string_view id);

bool Begin(const char* name, bool* p_open = nullptr,
           ImGuiWindowFlags flags = 0);

void End();

bool BeginChild(const char* str_id, const ImVec2& size = ImVec2(0, 0),
                bool border = false, ImGuiWindowFlags flags = 0);

void EndChild();

/**
 * Saves open status to storage "open" key.
 * If returning 'true' the header is open. doesn't indent nor push on ID stack.
 * user doesn't have to call TreePop().
 */
bool CollapsingHeader(const char* label, ImGuiTreeNodeFlags flags = 0);

bool TreeNodeEx(const char* label, ImGuiTreeNodeFlags flags = 0);

void TreePop();

// push string into the ID stack (will hash string).
void PushID(const char* str_id);

// push string into the ID stack (will hash string).
void PushID(const char* str_id_begin, const char* str_id_end);

// push string into the ID stack (will hash string).
inline void PushID(std::string_view str) {
  PushID(str.data(), str.data() + str.size());
}

// push integer into the ID stack (will hash integer).
void PushID(int int_id);

// pop from the ID stack.
void PopID();

bool PopupEditName(const char* label, std::string* name);

}  // namespace glass
