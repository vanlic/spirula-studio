// TrainPreset.cpp -- see TrainPreset.h.

#include "app/gui/TrainPreset.h"

#include "config/TrainConfigJson.h"

#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

namespace gui {

namespace {

// Every flag whose value differs from `ref`. Used when the file carries no
// `touched` list -- a run's config.json, or a preset from a build that
// predates the key -- so that loading one never lets a macro option quietly
// overwrite a value the file spelled out.
std::set<std::string> diff_fields(const TrainConfig& c, const TrainConfig& ref) {
    std::set<std::string> out;
#define SS_PRESET_DIFF(type, member, default_, section, tier, choices)        \
    if (!(c.member == ref.member)) out.insert(#member);
    SS_CONFIG_FIELDS(SS_PRESET_DIFF)
#undef SS_PRESET_DIFF
    // The context fields are not part of a preset, so a difference in one is
    // not tuning to protect.
#define SS_PRESET_DROP(member) out.erase(#member);
    SS_PRESET_CONTEXT_FIELDS(SS_PRESET_DROP)
#undef SS_PRESET_DROP
    return out;
}

void clear_context(TrainConfig& c) {
    const TrainConfig stock;
#define SS_PRESET_CLEAR(member) c.member = stock.member;
    SS_PRESET_CONTEXT_FIELDS(SS_PRESET_CLEAR)
#undef SS_PRESET_CLEAR
    if (c.lidar_only) {
        c.lidar_scan.clear();
        c.lidar_poses.clear();
    }
}

}  // namespace


void save_preset(const TrainPreset& p, const std::string& path) {
    TrainPreset copy = p;
    clear_context(copy.cfg);

    PresetHeader head{copy.name, copy.description, path};
    JsonWriter w = preset_writer(PresetKind::Train, head);
    w.field("base_preset", copy.base);
    w.key("touched").array();
    for (const std::string& t : copy.touched) w.value(t);
    w.end();
    w.key("config").object();
    for (const auto& [key, value] : train_config_json_pairs(copy.cfg))
        w.field_raw(key, value);
    w.end();
    w.end();
    write_preset_file(path, w.str());
}


TrainPreset load_preset(const std::string& path) {
    PresetHeader head;
    JsonValue root = read_preset_file(path, PresetKind::Train, head);

    TrainPreset p;
    p.path = path;
    p.name = head.name;
    p.description = head.description;

    const JsonValue* cfg_obj = root.find("config");
    const JsonValue& fields = (cfg_obj && cfg_obj->is_object()) ? *cfg_obj : root;
    if (!train_config_json_has_fields(fields))
        throw std::runtime_error(path + " holds no training options");

    auto str = [&](const char* key) -> std::string {
        const JsonValue* v = root.find(key);
        return v ? v->as_string() : std::string();
    };

    // Our own format names the built-in it started from "base_preset"; a run's
    // config.json names it "preset". Neither is required -- an unknown or
    // absent name just leaves the stock base, and the config it carries is
    // absolute anyway.
    p.base = str("base_preset");
    if (p.base.empty()) p.base = str("preset");
    TrainConfig base_cfg;
    if (p.base.empty() || !train_apply_preset(base_cfg, p.base)) {
        p.base = "3dgs";
        base_cfg = TrainConfig();
    }

    p.cfg = base_cfg;
    train_config_from_json(fields, p.cfg);   // throws on a malformed vec3
    clear_context(p.cfg);

    if (p.name.empty()) {
        // A run's config.json: the run folder is what anyone would call it.
        fs::path fp(path);
        p.name = fp.stem() == "config" && fp.has_parent_path()
                     ? fp.parent_path().filename().string()
                     : fp.stem().string();
    }

    if (const JsonValue* t = root.find("touched"); t && t->is_array()) {
        for (const JsonValue& e : t->arr)
            if (!e.as_string().empty()) p.touched.insert(e.as_string());
    } else {
        p.touched = diff_fields(p.cfg, base_cfg);
    }
    return p;
}


bool is_preset_file(const std::string& path) {
    try {
        (void)load_preset(path);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}


void delete_preset(const std::string& path) {
    // Deleting is the one operation here that destroys something, so it
    // checks what it is pointed at rather than trusting the caller: only a
    // file this module would read back as a preset can be removed through it.
    if (!is_preset_file(path))
        throw std::runtime_error(path + " is not a preset file");
    delete_preset_file(path, PresetKind::Train);
}


std::vector<TrainPreset> list_presets() {
    return list_preset_files(PresetKind::Train, load_preset);
}

}  // namespace gui
