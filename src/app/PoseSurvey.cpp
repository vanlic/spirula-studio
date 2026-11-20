#include "app/PoseSurvey.h"

#include "app/LidarAlign.h"
#include "data/DatasetParser.h"
#include "data/JsonWrite.h"
#include "data/LinkOrCopy.h"
#include "sfm/Pipeline.h"
#include "sfm/SfmConfig.h"
#include "sfm/map/Mapper.h"
#include "sfm/map/PhotoCoverage.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>

namespace app::survey {
namespace {

namespace fs = std::filesystem;
using namespace sfm;

constexpr double kPoseSigma = 0.05;
constexpr double kPlaneDistance = 0.25;
constexpr double kPlaneWeight = 20.0;

std::string trim(std::string s) {
    const auto start = s.find_first_not_of(" \r\n\t");
    return start == std::string::npos ? std::string{} : s.substr(start, s.find_last_not_of(" \r\n\t") - start + 1);
}

bool readLine(std::istream& in, std::string& line) {
    while (std::getline(in, line)) {
        if (line.compare(0, 3, "\xef\xbb\xbf") == 0) line.erase(0, 3);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const auto first = line.find_first_not_of(" \t");
        if (first != std::string::npos && line[first] != '#') return true;
    }
    return false;
}

char separator(const std::string& line) {
    std::array<int, 3> count{};
    constexpr std::array<char, 3> delimiters{',', '\t', ';'};
    bool quoted = false;
    for (char c : line) {
        if (c == '"') quoted = !quoted;
        if (quoted) continue;
        for (size_t i = 0; i < delimiters.size(); ++i) count[i] += c == delimiters[i];
    }
    const auto best = std::max_element(count.begin(), count.end());
    if (*best) return delimiters[best - count.begin()];
    return ' ';
}

std::vector<std::string> fields(const std::string& line, char sep) {
    std::vector<std::string> values;
    if (sep == ' ') {
        std::istringstream in(line);
        for (std::string value; in >> value;) values.push_back(std::move(value));
        return values;
    }
    std::string value;
    bool quoted = false;
    for (size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (quoted && c == '"' && i + 1 < line.size() && line[i + 1] == '"') {
            value += c;
            ++i;
        } else if (c == '"') quoted = !quoted;
        else if (!quoted && c == sep) {
            values.push_back(trim(value));
            value.clear();
        } else value += c;
    }
    if (quoted) throw std::runtime_error("Unclosed quote in pose table");
    values.push_back(trim(value));
    return values;
}

double number(const std::string& text) {
    size_t pos = 0;
    double value = std::stod(text, &pos);
    if (pos != text.size() || !std::isfinite(value)) throw std::runtime_error("Invalid number: " + text);
    return value;
}

std::string key(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    if (path == ".") return {};
    std::string result = fs::u8path(path).lexically_normal().generic_u8string();
    while (result.size() > 1 && result.back() == '/') result.pop_back();
    return result;
}

bool under(const std::string& path, const std::string& folder) {
    return folder.empty() || (path.size() > folder.size() &&
           path.compare(0, folder.size(), folder) == 0 && path[folder.size()] == '/');
}

std::vector<CameraOverride> readIntrinsics(const std::string& file, const fs::path& image_root) {
    std::ifstream in(fs::u8path(file));
    if (!in) throw std::runtime_error("Cannot read camera intrinsics: " + file);
    std::vector<CameraOverride> overrides;
    std::set<std::string> folders;
    size_t row = 0;
    for (std::string line; readLine(in, line);) {
        const auto values = fields(line, separator(line));
        const std::string where = file + ": data row " + std::to_string(++row);
        if (values.size() < 2 || values.front().empty())
            throw std::runtime_error(where + ": expected camera folder, model and parameters");
        CameraOverride entry;
        entry.prefix = key(values[0]);
        const auto relative = fs::u8path(entry.prefix);
        if (relative.has_root_path() || entry.prefix == ".." || under(entry.prefix, "..") ||
            !fs::is_directory(image_root / relative))
            throw std::runtime_error(where + ": camera folder must be inside the image directory");
        if (!folders.insert(entry.prefix).second)
            throw std::runtime_error(where + ": duplicate camera folder");
        std::string model = values[1];
        for (char& c : model) c = c == '_' ? '-' : char(std::tolower((unsigned char)c));
        entry.has_model = parseCamModelName(model, entry.model);
        if (!entry.has_model || !camInfo(entry.model).ba_refinable)
            throw std::runtime_error(where + ": unsupported camera model " + values[1]);
        const size_t n = camColmapParams(entry.model);
        if (values.size() != n + 2)
            throw std::runtime_error(where + ": expected " + std::to_string(n) + " COLMAP parameters for " + values[1]);
        for (size_t i = 0; i < n; ++i) entry.params.push_back(number(values[i + 2]));
        for (int i = 0; i < camInfo(entry.model).ba_focal; ++i)
            if (!(entry.params[i] > 0)) throw std::runtime_error(where + ": focal lengths must be positive");
        overrides.push_back(std::move(entry));
    }
    if (overrides.empty()) throw std::runtime_error("No camera intrinsics in " + file);
    return overrides;
}

std::string numericKey(const std::string& text) {
    if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos) return text;
    const auto first = text.find_first_not_of('0');
    return first == std::string::npos ? "0" : text.substr(first);
}

Pose makePose(const double* q, const double* t) {
    const double norm = std::hypot(std::hypot(q[0], q[1]), std::hypot(q[2], q[3]));
    if (norm < 1e-12 || !std::isfinite(norm)) throw std::runtime_error("Invalid pose quaternion");
    return {quaternionToRotation({q[3]/norm, q[0]/norm, q[1]/norm, q[2]/norm}), {t[0], t[1], t[2]}};
}

void readPoses(const PoseTable& source, const MatchesDatabase& db, std::map<uint32_t, Pose>& out) {
    std::ifstream in(fs::u8path(source.file));
    std::string line;
    if (!readLine(in, line)) throw std::runtime_error("Cannot read pose table: " + source.file);
    const char sep = separator(line);
    const int last = *std::max_element(source.columns.begin(), source.columns.end());
    std::map<std::string, uint32_t> ids;
    auto index = [&](const std::string& name, uint32_t id) {
        const auto [it, inserted] = ids.emplace(name, id);
        if (!inserted && it->second != id) it->second = UINT32_MAX;
    };
    const std::string prefix = key(source.camera);
    for (uint32_t i = 0; i < db.images.size(); ++i) {
        const std::string name = key(db.images[i].name);
        if (!under(name, prefix)) continue;
        index(name, i);
        if (!prefix.empty()) index(name.substr(prefix.size() + 1), i);
        const std::string stem = fs::u8path(name).filename().u8string();
        index(numericKey(stem), i);
        index(stem, i);
    }
    size_t count = 0, missing = 0, row = 0;
    do {
        if (++row == 1 && source.header) continue;
        const auto v = fields(line, sep);
        if (v.size() <= size_t(last)) throw std::runtime_error(source.file + ": short row " + std::to_string(row));
        const std::string name = key(v[source.columns[0]]);
        auto it = ids.find(name);
        if (it == ids.end()) {
            auto stem = fs::u8path(name);
            stem.replace_extension();
            it = ids.find(stem.generic_u8string());
        }
        if (it == ids.end()) it = ids.find(numericKey(name));
        if (it == ids.end()) { ++missing; continue; }
        if (it->second == UINT32_MAX) throw std::runtime_error("Ambiguous image ID: " + name);
        double p[7];
        for (int j = 0; j < 7; ++j) p[j] = number(v[source.columns[j + 1]]);
        Pose pose = makePose(p, p + 4);
        if (!source.world_to_camera) pose = invertPose(pose);
        if (!out.emplace(it->second, pose).second) throw std::runtime_error("Duplicate pose for " + db.images[it->second].name);
        ++count;
    } while (readLine(in, line));
    if (!count) throw std::runtime_error("No image ID matched: " + source.file);
    std::printf("[poses] %zu matched, %zu unmatched in %s\n", count, missing, source.file.c_str());
}

class MetricPriors final : public PriorSource {
public:
    MetricPriors(const std::map<uint32_t, Pose>& poses, bool quadratic)
        : poses_(poses), quadratic_(quadratic) {}
    bool has(uint32_t i) const override { return poses_.count(i) != 0; }
    bool relativeRotation(uint32_t, uint32_t, Mat3&, double&) const override { return false; }
    std::vector<uint32_t> neighbours(uint32_t) const override { return {}; }
    PosePriors factors(const std::vector<PosedImage>& images) override {
        PosePriors result;
        if (quadratic_) result.huber = std::numeric_limits<double>::infinity();
        for (const auto& image : images) {
            auto it = poses_.find(image.image);
            if (it == poses_.end()) continue;
            PriorCentre p;
            p.n = 1;
            p.img[0] = image.image;
            p.b = cameraCenter(it->second);
            p.sigma = {kPoseSigma, kPoseSigma, kPoseSigma};
            result.centres.push_back(p);
        }
        return result;
    }
private:
    const std::map<uint32_t, Pose>& poses_;
    bool quadratic_;
};

void completeRig(const RigTable& rigs, std::map<uint32_t, Pose>& poses) {
    for (const auto& rig : rigs.rigs) {
        for (const auto& frame : rig.frames) {
            auto at = poses.find(frame[0]);
            if (at != poses.end() && frame[1] != kNoImage) {
                poses.insert_or_assign(frame[1], composePose(rig.members[1].ext, at->second));
            } else if (auto other = poses.find(frame[1]); other != poses.end() && frame[0] != kNoImage) {
                poses.emplace(frame[0], composePose(invertPose(rig.members[1].ext), other->second));
            }
        }
    }
}

bool prepareTrainingFile(const fs::path& source, const fs::path& output) {
    cancel::check();
    if (!fs::is_regular_file(source)) throw std::runtime_error("Missing training file: " + source.u8string());
    if (fs::exists(output)) {
        if (fs::is_regular_file(output) && fs::equivalent(source, output)) return true;
        if (fs::is_regular_file(output) && fs::file_size(source) == fs::file_size(output)) {
            std::ifstream a(source, std::ios::binary), b(output, std::ios::binary);
            std::array<char, 65536> x{}, y{};
            while (a && b) {
                a.read(x.data(), x.size());
                b.read(y.data(), y.size());
                if (a.gcount() != b.gcount() || !std::equal(x.begin(), x.begin() + a.gcount(), y.begin())) break;
                if (a.eof() && b.eof() && !a.bad() && !b.bad()) return true;
            }
        }
        throw std::runtime_error("Training output conflicts with existing file: " + output.u8string());
    }
    fs::create_directories(output.parent_path());
    std::error_code error;
    if (spirula::files::link_or_copy(source, output, error) == spirula::files::Transfer::Failed)
        throw std::runtime_error("Cannot prepare " + output.u8string() + ": " + error.message());
    return false;
}

struct TrainingFiles {
    size_t images_reused = 0, masks_ready = 0;
};

TrainingFiles exportTrainingFiles(const Reconstruction& model, const fs::path& images,
                                  const std::string& masks, const fs::path& output) {
    std::vector<std::string> names;
    for (const auto& [id, image] : model.images) if (image.registered) names.push_back(image.name);
    const auto associated = masks.empty() ? std::vector<std::string>(names.size())
                                          : dsparse::find_aux_files(masks, names, "mask");
    const bool reuse_masks = !masks.empty() && fs::is_directory(output / "masks") &&
                             fs::equivalent(fs::u8path(masks), output / "masks");
    TrainingFiles files;
    for (size_t i = 0; i < names.size(); ++i) {
        const fs::path rel = fs::u8path(names[i]).lexically_normal();
        if (rel.empty() || rel.has_root_path() || *rel.begin() == "..")
            throw std::runtime_error("Invalid relative image name: " + names[i]);
        files.images_reused += prepareTrainingFile(images / rel, output / "images" / rel);
        if (associated[i].empty()) continue;
        ++files.masks_ready;
        if (reuse_masks) continue;
        const fs::path mask = fs::u8path(associated[i]);
        fs::path target = output / "masks" / rel;
        target += mask.extension();
        prepareTrainingFile(mask, target);
    }
    std::printf("[dataset] %zu images (%zu reused), %zu masks ready for training\n",
                names.size(), files.images_reused, files.masks_ready);
    return files;
}

void writeReport(const fs::path& path, const JsonWriter& report) {
    std::ofstream out(path, std::ios::binary);
    out << report.str();
    out.close();
    if (!out) throw std::runtime_error("Cannot write BA report: " + path.u8string());
}

}  // namespace

std::vector<std::string> previewColumns(const std::string& path, bool header) {
    std::ifstream in(fs::u8path(path));
    std::string line;
    if (!readLine(in, line)) throw std::runtime_error("Cannot read pose table: " + path);
    auto values = fields(line, separator(line));
    if (!header) for (size_t i = 0; i < values.size(); ++i) values[i] = std::to_string(i + 1) + "  " + values[i];
    return values;
}

int runCommand(int argc, char** argv) {
    std::string images, output, scan, flat, masks, rig_file, intrinsics_file;
    std::array<double, 7> rig_transform{};
    std::vector<PoseTable> tables;
    SfmConfig cfg;
    bool planes = false;
    double overlap = 0, focal = 0;
    for (int i = 0; i < argc; ++i) {
        const std::string arg = argv[i];
        auto next = [&]() -> std::string {
            if (++i >= argc) throw std::runtime_error(arg + " needs a value");
            return argv[i];
        };
        if (arg == "--images") images = next();
        else if (arg == "--output") output = next();
        else if (arg == "--masks") masks = next();
        else if (arg == "--scan") scan = next();
        else if (arg == "--plane-ba") planes = true;
        else if (arg == "--flat") flat = key(next());
        else if (arg == "--replace") overlap = number(next());
        else if (arg == "--focal") focal = number(next());
        else if (arg == "--intrinsics") intrinsics_file = next();
        else if (arg == "--poses") {
            PoseTable table;
            table.file = next(); table.camera = next();
            table.header = number(next()) != 0;
            table.world_to_camera = number(next()) != 0;
            std::set<int> used;
            for (int& col : table.columns) {
                const double v = number(next());
                if (v < 0 || v > 10000 || std::floor(v) != v || !used.insert(int(v)).second)
                    throw std::runtime_error("Pose columns must be distinct zero-based indices");
                col = int(v);
            }
            tables.push_back(std::move(table));
        } else if (arg == "--rig") {
            if (!cfg.rigs.empty()) throw std::runtime_error("Only one dual-fisheye rig is supported");
            RigDef rig;
            rig.name = "survey"; rig.kind = "dual-fisheye";
            const std::string first = key(next()), second = key(next());
            if (first.empty() || second.empty() || under(first, second) || under(second, first) || first == second)
                throw std::runtime_error("Choose distinct folders for the two rig members");
            rig_file = next();
            std::ifstream in(fs::u8path(rig_file));
            std::string line;
            if (!readLine(in, line)) throw std::runtime_error("Cannot read rig transform: " + rig_file);
            const auto values = fields(line, separator(line));
            if (values.size() != 7) throw std::runtime_error("Rig transform needs tx ty tz qx qy qz qw");
            auto* p = rig_transform.data();
            for (int j = 0; j < 7; ++j) p[j] = number(values[j]);
            rig.members = {{first, true, {mat3Identity(), {}}, true, kRigDofNone},
                           {second, true, makePose(p + 3, p), false, kRigDofAll}};
            cfg.rigs.push_back(std::move(rig));
        } else if (arg == "--help") {
            std::puts("spirula sfm survey --images DIR --output DIR --poses FILE PREFIX HEADER W2C ID QX QY QZ QW X Y Z\n"
                      "Columns are zero based. HEADER and W2C are 0 or 1; repeat --poses for more tables.\n"
                      "Options: --masks DIR --scan FILE --plane-ba --focal PX | --intrinsics FILE\n"
                      "         --rig FIRST SECOND TRANSFORM --flat PREFIX --replace FRACTION");
            std::puts("The intrinsics TXT lists the camera folder, COLMAP model and source-pixel parameters.\n"
                      "Whitespace-separated example for two cameras");
            std::puts(kIntrinsicsExample);
            return 0;
        } else throw std::runtime_error("Unknown option: " + arg);
    }
    if (images.empty() || output.empty() || tables.empty()) throw std::runtime_error("Images, output and pose tables are required");
    if ((planes && scan.empty()) || overlap < 0 || overlap > 1 || (overlap > 0 && flat.empty()) || focal < 0)
        throw std::runtime_error("Invalid scan, focal length or photo overlap options");
    if (!flat.empty() && (fs::u8path(flat).has_root_path() || flat == ".." || under(flat, "..") ||
        !fs::is_directory(fs::u8path(images) / fs::u8path(flat))))
        throw std::runtime_error("The flat-photo folder must be inside the image directory");
    if (!flat.empty()) for (const auto& rig : cfg.rigs) for (const auto& member : rig.members)
        if (flat == member.prefix || under(flat, member.prefix) || under(member.prefix, flat))
            throw std::runtime_error("Detail photos and rig lenses need separate folders");
    const fs::path root = fs::u8path(output);
    if (fs::exists(root / "sparse")) throw std::runtime_error("The output already contains a sparse model");
    if (fs::exists(root / kReportFile)) throw std::runtime_error("The output already contains a BA report");
    const bool calibrated = !intrinsics_file.empty();
    const fs::path features = root / (calibrated ? ".survey-features" : ".survey-features-estimated");
    cfg.image_dir = images;
    cfg.mask_dir = masks;
    cfg.camera_mode_pinned = true;
    cfg.camera_model = "thin-prism-fisheye";
    const bool has_rig = !cfg.rigs.empty();
    if (calibrated) {
        if (focal > 0) throw std::runtime_error("Use either --intrinsics or --focal");
        cfg.camera.overrides = readIntrinsics(intrinsics_file, fs::u8path(images));
        if (has_rig) for (const auto& member : cfg.rigs.front().members) {
            const auto it = std::find_if(cfg.camera.overrides.begin(), cfg.camera.overrides.end(),
                                         [&](const auto& row) { return row.prefix == member.prefix; });
            if (it == cfg.camera.overrides.end() || (it->model != CamModel::ThinPrismFisheye &&
                                                    it->model != CamModel::OpenCVFisheye))
                throw std::runtime_error("Provide fisheye intrinsics for rig folder: " + member.prefix);
        }
        cfg.mapper.pp_min_images = 3;
    } else {
        cfg.max_image_size = 1600;
        cfg.sift.max_num_features = 6000;
        cfg.max_error = 4.0;
        cfg.mapper.min_tri_angle_deg = 0.5;
    }
    if (focal > 0) {
        CameraOverride fish;
        fish.has_focal = true;
        fish.focal = focal;
        cfg.camera.overrides.push_back(fish);
    }
    if (!flat.empty() && std::none_of(cfg.camera.overrides.begin(), cfg.camera.overrides.end(),
                                     [&](const auto& row) { return row.prefix == flat; })) {
        CameraOverride camera;
        camera.prefix = flat;
        camera.model = CamModel::OpenCV;
        camera.has_model = true;
        cfg.camera.overrides.push_back(camera);
    }
    cfg.sensor_map = cfg.sensor_verify = false;
    cfg.mapper.ba_real = "double";
    cfg.mapper.refine_principal_point = true;
    cfg.mapper.verbose = true;
    if (const std::string error = cfg.finalize(CMD_AUTO); !error.empty()) throw std::runtime_error(error);

    std::vector<MetricPoseBaStage> stages;
    if (calibrated) {
        stages.push_back({"initial_poses", true, false, 2, 100, 12, 12, false});
        if (has_rig) stages.push_back({"relative_rig", true, true, 2, 100, 4, 12, true});
        stages.push_back({"joint_calibration", false, has_rig, 3, 100, 4, 4, true});
    } else {
        const auto& m = cfg.mapper;
        stages.push_back({"initial_poses", false, false, m.ba_max_refinements, 0,
                          m.ba_loss_param, m.max_reproj_error, false});
        stages.push_back({has_rig ? "relative_rig" : "refine_poses", false, has_rig,
                          m.ba_max_refinements, 0, m.ba_loss_param, m.max_reproj_error, false});
    }
    const bool joint_planes = planes && calibrated;
    JsonWriter report;
    report.object().field("version", 2).key("configuration").object()
        .field("images", fs::absolute(fs::u8path(images)).u8string())
        .field("masks", masks.empty() ? masks : fs::absolute(fs::u8path(masks)).u8string())
        .field("scan", scan.empty() ? scan : fs::absolute(fs::u8path(scan)).u8string())
        .field("camera_model", cfg.camera_model)
        .field("focal_override_px", focal)
        .field("max_image_size", cfg.max_image_size).field("max_features", cfg.sift.max_num_features)
        .field("detail_photo_folder", flat)
        .field("remove_fisheye_overlap_fraction", overlap)
        .key("pose_tables").array();
    static constexpr const char* roles[] = {"id", "qx", "qy", "qz", "qw", "x", "y", "z"};
    for (const auto& table : tables) {
        report.object().field("file", fs::absolute(fs::u8path(table.file)).u8string())
            .field("camera_folder", table.camera).field("header", table.header)
            .field("pose_direction", table.world_to_camera ? "world_to_camera" : "camera_to_world")
            .key("zero_based_columns").object();
        for (size_t i = 0; i < table.columns.size(); ++i) report.field(roles[i], table.columns[i]);
        report.end().end();
    }
    report.end().key("camera_intrinsics");
    if (!calibrated) report.raw("null");
    else {
        report.object().field("file", fs::absolute(fs::u8path(intrinsics_file)).u8string())
            .key("calibrations").array();
        for (const auto& row : cfg.camera.overrides) {
            if (row.params.empty()) continue;
            report.object().field("folder", row.prefix.empty() ? "." : row.prefix)
                .field("model", camInfo(row.model).cli_name).key("colmap_parameters").array();
            for (double v : row.params) report.raw(json_number_exact(v));
            report.end().end();
        }
        report.end().end();
    }
    report.key("dual_fisheye_rig");
    if (cfg.rigs.empty()) report.raw("null");
    else {
        const auto& members = cfg.rigs.front().members;
        report.object().field("file", fs::absolute(fs::u8path(rig_file)).u8string())
            .field("first_camera_folder", members[0].prefix).field("second_camera_folder", members[1].prefix)
            .field("refine_extrinsics", true).key("first_to_second_tx_ty_tz_qx_qy_qz_qw").array();
        for (double value : rig_transform) report.raw(json_number_exact(value));
        report.end().end();
    }
    const auto& mapping = cfg.mapper;
    report.key("visual_ba").object().field("center_prior_sigma_m", kPoseSigma)
        .field("center_prior_loss", calibrated ? "huber" : "quadratic")
        .field("mode", calibrated ? "supplied_intrinsics" : "estimated_intrinsics")
        .field("real", mapping.ba_real).field("coarse_real", mapping.ba_real_coarse)
        .field("solver", mapping.ba_solver).field("loss", mapping.ba_loss)
        .field("loss_parameter_extraction_px", mapping.ba_loss_param)
        .field("max_reprojection_error_extraction_px", mapping.max_reproj_error)
        .field("min_triangulation_angle_deg", mapping.min_tri_angle_deg)
        .field("refine_principal_point", mapping.refine_principal_point)
        .field("principal_point_min_images", (long long)mapping.pp_min_images)
        .field("refine_distortion", mapping.refine_extra_params)
        .field("final_tight", mapping.ba_final_tight)
        .field("final_refinements", mapping.ba_max_refinements)
        .field("final_prior_max_iterations", mapping.ba_final_prior_max_iters)
        .field("final_prior_gradient_tolerance", mapping.ba_final_prior_gradient_tol)
        .key("stages").array();
    for (const auto& stage : stages) {
        report.object().field("name", stage.name).field("refine_intrinsics", !stage.fix_intrinsics)
            .field("refine_rig_extrinsics", stage.refine_rigs).field("max_rounds", stage.rounds)
            .field("max_iterations_per_round", stage.max_iters ? stage.max_iters : mapping.ba_final_prior_max_iters)
            .field("rebuild_tracks", stage.rebuild_tracks)
            .field("max_reprojection_error_extraction_px", stage.reproj_error)
            .field("loss_parameter_extraction_px", stage.loss_param).end();
    }
    report.end().end();
    BundleOptions plane_bundle;
    plane_bundle.refine_rigs = false;
    auto plane_options = bundleSolverOptions(plane_bundle);
    plane_options.verbose = true;
    report.key("point_to_plane_ba").object().field("enabled", planes);
    if (planes) {
        report.field("max_plane_match_distance_m", kPlaneDistance).field("sqrt_weight", kPlaneWeight)
            .field("stage", joint_planes ? stages.back().name : "scan_refinement")
            .field("refine_intrinsics", joint_planes).field("refine_rig_extrinsics", joint_planes && has_rig);
        if (!joint_planes) report.field("fix_rotations", plane_options.fix_rotations)
            .field("real", realCfgName(plane_options.real)).field("solver", plane_bundle.solver)
            .field("loss", plane_options.loss).field("loss_parameter_image_px", plane_options.loss_param)
            .field("max_iterations", plane_options.max_iters).field("relative_tolerance", plane_options.rtol)
            .field("patience", plane_options.patience).field("initial_damping", plane_options.init_damping);
    }
    report.end().end();

    ExtractStats extracted;
    if (extractDirectory(images, features, cfg, extracted, true)) return 1;
    std::vector<FeatureSet> feats;
    MatchesDatabase db;
    MatchStats matched;
    VerifyCalibration calibration;
    calibration.setup = cfg.camera;
    calibration.image_dir = images;
    if (matchFeatureDir(features.u8string(), cfg,
                        extracted.images < 100 ? PairMode::Exhaustive : PairMode::Prefilter,
                        true, feats, db, matched, &calibration)) return 1;
    const auto& cameras = calibration.cameras;
    cfg.mapper.initial_cameras = cameras.cameras;
    cfg.mapper.known_focal_cameras = cameras.focal_known;
    cfg.mapper.given_focal_cameras = cameras.focal_given;
    cfg.mapper.measured_focal_cameras = cameras.focal_measured;
    const RigTable rigs = buildRigs(db, cfg, true);
    std::map<uint32_t, Pose> poses;
    for (const auto& table : tables) readPoses(table, db, poses);
    if (poses.size() < 2) throw std::runtime_error("At least two independently posed images are required");
    auto initial = poses;
    completeRig(rigs, initial);
    poses.insert(initial.begin(), initial.end());
    MetricPriors priors(poses, !calibrated);
    size_t plane_count = 0;
    BundleRun plane_run;
    std::optional<app::lidar::AlignCloud> plane_cloud;
    auto attachPlanes = [&](BAProblem& problem) {
        if (!plane_cloud) plane_cloud.emplace(app::lidar::load_scan_planes(scan));
        plane_count = app::lidar::attach_scan_planes(*plane_cloud, problem.points, kPlaneDistance,
                                                     kPlaneWeight, problem.point_planes);
        if (!plane_count) throw std::runtime_error("No scan planes match the model. Check the coordinate frame");
        std::printf("[lidar] %zu scan planes\n", plane_count);
    };
    if (joint_planes) stages.back().prepare = attachPlanes;
    Mapper mapper(db, feats, cfg.mapper, cameras.ids, &rigs, nullptr, &priors);
    Reconstruction model = mapper.fromMetricPoses(initial, stages);
    if (model.numRegistered() < 2 || model.points3D.empty())
        throw std::runtime_error("No tracks triangulate from the supplied poses and verified matches");
    if (joint_planes) plane_run = stages.back().last;
    else if (planes) {
        std::vector<PosedImage> posed;
        for (const auto& [id, image] : model.images)
            if (image.registered) posed.push_back({id, image.camera_id, image.pose});
        const auto centres = priors.factors(posed);
        plane_bundle.priors = &centres;
        plane_bundle.rigs = &rigs;
        auto layout = buildBundle(model, plane_bundle, true);
        layout.attachPriors();
        attachPlanes(layout.P);
        plane_run = solveBundle(layout.P, plane_options, nullptr);
        writeBundle(model, layout, layout.P);
    }
    size_t removed = 0;
    if (overlap > 0) {
        std::set<uint32_t> detail;
        for (const auto& [id, image] : model.images)
            if (image.registered && under(image.name, flat)) detail.insert(id);
        removed = removeCoveredFisheyes(model, detail, overlap);
        std::printf("[photos] %zu covered fisheye views removed\n", removed);
    }
    if (model.numRegistered() < 2 || model.points3D.empty()) throw std::runtime_error("No triangulated model remains");
    std::vector<Reconstruction> models;
    models.push_back(std::move(model));
    resolveImageNames(models, images);
    if (std::any_of(models[0].images.begin(), models[0].images.end(), [&](const auto& item) {
        return item.second.registered && !fs::is_regular_file(fs::u8path(images) / fs::u8path(item.second.name));
    })) throw std::runtime_error("Model image names could not be resolved to the source files");
    ModelGauge gauge;
    gauge.metric = true;
    gauge.scale = "trajectory";
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path pending = root / (".survey-result-" + std::to_string(stamp));
    if (!fs::create_directory(pending)) throw std::runtime_error("Cannot reserve model output");
    std::vector<ReprojectionStats> reprojection;
    writeModels(models, pending, false, {gauge}, &rigs, &reprojection);
    const auto files = exportTrainingFiles(models[0], fs::u8path(images), masks, root);
    size_t details = 0;
    for (const auto& [id, image] : models[0].images)
        details += image.registered && !flat.empty() && under(image.name, flat);
    const auto& errors = reprojection.front();
    report.key("result").object().field("input_images", (long long)db.images.size())
        .field("registered_images", (long long)models[0].numRegistered())
        .field("registered_detail_photos", (long long)details).field("removed_fisheye_images", (long long)removed)
        .field("reused_images", (long long)files.images_reused).field("exported_masks", (long long)files.masks_ready)
        .field("points_3d", (long long)models[0].points3D.size())
        .key("reprojection_error_px").object().field("observations", (long long)errors.measured)
        .field("skipped_observations", (long long)errors.skipped)
        .field_raw("mean", errors.measured ? json_number(errors.mean) : "null")
        .field_raw("max", errors.measured ? json_number(errors.max) : "null")
        .field_raw("median", errors.measured ? json_number(errors.median) : "null")
        .end().key("visual_ba_stages").array();
    for (const auto& stage : stages) {
        report.object().field("name", stage.name).field("solves", stage.solves)
            .field("pose_blocks", (long long)stage.last.pose_blocks)
            .field("rig_dof", (long long)stage.last.rig_dof).field("intrinsic_dof", (long long)stage.last.intrinsic_dof)
            .field("real", realCfgName(stage.last.real)).field("solver", stage.last.stats.solver).end();
    }
    report.end();
    if (planes) {
        report.key("point_to_plane_ba").object().field("planes", (long long)plane_count)
            .field("real", realCfgName(plane_run.real)).field("solver", plane_run.stats.solver)
            .field("iterations", plane_run.stats.iterations).field("accepted_steps", plane_run.stats.accepted).end();
    }
    report.end().end();
    const fs::path report_path = root / kReportFile;
    const fs::path staged_report = pending / kReportFile;
    writeReport(staged_report, report);
    cancel::check();
    std::error_code publish_error;
    if (spirula::files::link_or_copy(staged_report, report_path, publish_error) == spirula::files::Transfer::Failed)
        throw std::runtime_error("Cannot publish BA report: " + publish_error.message());
    try {
        fs::remove(staged_report);
        fs::rename(pending, root / "sparse");
    } catch (...) {
        std::error_code error;
        fs::remove(report_path, error);
        throw;
    }
    if (errors.measured) std::printf("[model] reprojection error %.4f mean, %.4f max, %.4f median px (%zu observations)\n",
                                    errors.mean, errors.max, errors.median, errors.measured);
    std::printf("[model] %u/%zu registered images, %zu detail photos\n%s\n", models[0].numRegistered(),
                db.images.size(), details, (root / "sparse" / "0").u8string().c_str());
    std::printf("[report] %s\n", report_path.u8string().c_str());
    return 0;
}

}  // namespace app::survey
