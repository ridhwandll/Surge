// Copyright (c) - SurgeTechnologies - All rights reserved
#include "StorageBinding.hpp"
#include "Surge/ScriptEngine/Lua.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Core/Client.hpp"
#include "Surge/Utility/Platform.hpp"

#include <json/json.hpp>
#include <glm/glm.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>

// Persistent save data for scripts. One JSON file per key in <persistent storage>/<ProjectName>/SaveData/, written to a
// temporary file first and renamed, so a crash mid-write never leaves a half written save. Plain std::filesystem/fstream:
// the engine's Filesystem reads the (read only) APK on Android, saves live in the app's internal storage instead
namespace Surge::ScriptBinding
{
    namespace fs = std::filesystem;

    static constexpr int MAX_DEPTH = 32;
    static constexpr size_t MAX_KEY_LENGTH = 64;

    static bool IsSafeChar(char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
    }

    static bool IsValidKey(const String& key)
    {
        if(key.empty() || key.size() > MAX_KEY_LENGTH || key[0] == '.')
            return false;
        for(char c : key)
        {
            if(!IsSafeChar(c))
                return false;
        }
        return true;
    }

    static fs::path SaveDirectory()
    {
        String project = Core::GetClient() ? Core::GetClient()->GetProject().Name : String();
        for(char& c : project)
        {
            if(!IsSafeChar(c))
                c = '_';
        }
        if(project.empty())
            project = "Default";

        const fs::path dir = fs::path(Platform::GetPersistantStoragePath()) / project / "SaveData";
        std::error_code ec;
        fs::create_directories(dir, ec);
        return dir;
    }

    static fs::path KeyPath(const String& key)
    {
        return SaveDirectory() / (key + ".json");
    }

    // Lua -> JSON ------------------------------------------------------------------------------
    static bool IsLuaInteger(const sol::object& object)
    {
        lua_State* L = object.lua_state();
        object.push(L);
        const bool isInteger = lua_isinteger(L, -1) != 0;
        lua_pop(L, 1);
        return isInteger;
    }

    static bool ToJson(const sol::object& object, nlohmann::json& out, const String& path, int depth, String& error)
    {
        if(depth > MAX_DEPTH)
        {
            error = path + " is nested too deep (cyclic table?)";
            return false;
        }

        switch(object.get_type())
        {
            case sol::type::lua_nil: out = nullptr; return true;
            case sol::type::boolean: out = object.as<bool>(); return true;
            case sol::type::string: out = object.as<String>(); return true;
            case sol::type::number:
                if(IsLuaInteger(object))
                    out = object.as<int64_t>();
                else
                    out = object.as<double>();
                return true;
            case sol::type::userdata:
                if(object.is<glm::vec2>()) { const glm::vec2 v = object.as<glm::vec2>(); out = { { "$vec2", { v.x, v.y } } }; return true; }
                if(object.is<glm::vec3>()) { const glm::vec3 v = object.as<glm::vec3>(); out = { { "$vec3", { v.x, v.y, v.z } } }; return true; }
                if(object.is<glm::vec4>()) { const glm::vec4 v = object.as<glm::vec4>(); out = { { "$vec4", { v.x, v.y, v.z, v.w } } }; return true; }
                error = path + " is a userdata that cannot be saved (only Math.Vec2/Vec3/Vec4 can)";
                return false;
            case sol::type::table:
            {
                // A sequence 1..n is saved as a JSON array, anything else as an object with string keys
                const sol::table table = object.as<sol::table>();
                Vector<std::pair<sol::object, sol::object>> entries;
                for(const auto& [key, value] : table)
                    entries.emplace_back(key, value);

                bool isArray = true;
                for(const auto& [key, value] : entries)
                {
                    if(key.get_type() != sol::type::number || !IsLuaInteger(key))
                    {
                        isArray = false;
                        break;
                    }
                    const int64_t index = key.as<int64_t>();
                    if(index < 1 || index > static_cast<int64_t>(entries.size()))
                    {
                        isArray = false;
                        break;
                    }
                }

                if(isArray && !entries.empty())
                {
                    out = nlohmann::json::array();
                    out.get_ref<nlohmann::json::array_t&>().resize(entries.size());
                    for(const auto& [key, value] : entries)
                    {
                        const int64_t index = key.as<int64_t>();
                        if(!ToJson(value, out[static_cast<size_t>(index - 1)], path + "[" + std::to_string(index) + "]", depth + 1, error))
                            return false;
                    }
                    return true;
                }

                out = nlohmann::json::object();
                for(const auto& [key, value] : entries)
                {
                    String keyString;
                    if(key.get_type() == sol::type::string)
                        keyString = key.as<String>();
                    else if(key.get_type() == sol::type::number)
                        keyString = IsLuaInteger(key) ? std::to_string(key.as<int64_t>()) : std::to_string(key.as<double>());
                    else
                    {
                        error = path + " has a key that is not a string or number";
                        return false;
                    }
                    if(!ToJson(value, out[keyString], path + "." + keyString, depth + 1, error))
                        return false;
                }
                return true;
            }
            default:
                error = path + " is a " + sol::type_name(object.lua_state(), object.get_type()) + ", only nil, boolean, number, string, table and Math.Vec2/Vec3/Vec4 can be saved";
                return false;
        }
    }

    // JSON -> Lua ------------------------------------------------------------------------------
    static sol::object ToLua(const nlohmann::json& json, sol::state_view lua)
    {
        switch(json.type())
        {
            case nlohmann::json::value_t::null: return sol::make_object(lua, sol::lua_nil);
            case nlohmann::json::value_t::boolean: return sol::make_object(lua, json.get<bool>());
            case nlohmann::json::value_t::number_integer:
            case nlohmann::json::value_t::number_unsigned: return sol::make_object(lua, json.get<int64_t>());
            case nlohmann::json::value_t::number_float: return sol::make_object(lua, json.get<double>());
            case nlohmann::json::value_t::string: return sol::make_object(lua, json.get<String>());
            case nlohmann::json::value_t::array:
            {
                sol::table table = lua.create_table(static_cast<int>(json.size()), 0);
                for(size_t i = 0; i < json.size(); i++)
                    table.raw_set(static_cast<int64_t>(i + 1), ToLua(json[i], lua));
                return table;
            }
            case nlohmann::json::value_t::object:
            {
                if(json.size() == 1)
                {
                    const auto vec = json.begin();
                    const nlohmann::json& v = vec.value();
                    if(vec.key() == "$vec2" && v.is_array() && v.size() == 2) return sol::make_object(lua, glm::vec2(v[0].get<float>(), v[1].get<float>()));
                    if(vec.key() == "$vec3" && v.is_array() && v.size() == 3) return sol::make_object(lua, glm::vec3(v[0].get<float>(), v[1].get<float>(), v[2].get<float>()));
                    if(vec.key() == "$vec4" && v.is_array() && v.size() == 4) return sol::make_object(lua, glm::vec4(v[0].get<float>(), v[1].get<float>(), v[2].get<float>(), v[3].get<float>()));
                }
                sol::table table = lua.create_table(0, static_cast<int>(json.size()));
                for(const auto& [key, value] : json.items())
                    table.raw_set(key, ToLua(value, lua));
                return table;
            }
            default: return sol::make_object(lua, sol::lua_nil);
        }
    }

    void BindStorage(void* luaState)
    {
        sol::state_view& lua = *static_cast<sol::state_view*>(luaState);
        sol::table storage = lua.create_named_table("Storage");

        // Storage.Save("progress", { level = 3, coins = 120 }) -> true on success. Small values, not every frame (file IO)
        storage["Save"] = [](const String& key, sol::object value) -> bool {
            if(!IsValidKey(key))
            {
                Log<Severity::Warn>("[Storage] Save: invalid key '{}' (1-64 characters: letters, digits, _ - .)", key);
                return false;
            }

            nlohmann::json json;
            String error;
            if(!ToJson(value, json, key, 0, error))
            {
                Log<Severity::Warn>("[Storage] Save('{}') failed: {}", key, error);
                return false;
            }

            const fs::path path = KeyPath(key);
            const fs::path temp = fs::path(path).concat(".tmp");
            {
                std::ofstream file(temp, std::ios::binary | std::ios::trunc);
                if(!file)
                {
                    Log<Severity::Error>("[Storage] Save('{}'): cannot write {}", key, temp.string());
                    return false;
                }
                file << json.dump();
                if(!file.good())
                    return false;
            }

            std::error_code ec;
            fs::rename(temp, path, ec);
            if(ec)
            {
                Log<Severity::Error>("[Storage] Save('{}'): {}", key, ec.message());
                return false;
            }
            return true;
        };

        // Storage.Load("progress", defaultValue) -> the saved value, or defaultValue (nil) when there is none
        storage["Load"] = [](const String& key, sol::object fallback, sol::this_state s) -> sol::object {
            if(!IsValidKey(key))
            {
                Log<Severity::Warn>("[Storage] Load: invalid key '{}'", key);
                return fallback;
            }

            std::ifstream file(KeyPath(key), std::ios::binary);
            if(!file)
                return fallback;

            std::stringstream text;
            text << file.rdbuf();
            const nlohmann::json json = nlohmann::json::parse(text.str(), nullptr, false);
            if(json.is_discarded())
            {
                Log<Severity::Warn>("[Storage] Load('{}'): the save file is corrupted, returning the default", key);
                return fallback;
            }
            return ToLua(json, sol::state_view(s));
        };

        storage["Has"] = [](const String& key) -> bool {
            std::error_code ec;
            return IsValidKey(key) && fs::exists(KeyPath(key), ec);
        };

        storage["Delete"] = [](const String& key) -> bool {
            std::error_code ec;
            return IsValidKey(key) && fs::remove(KeyPath(key), ec);
        };
    }
}
