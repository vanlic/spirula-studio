#include "app/gui/PoseSurveyPanel.h"

#ifdef SS_TOOL_SFM
#include "app/AppPaths.h"
#include "app/gui/Ui.h"
#include "i18n/catalog/PoseSurvey.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <set>

namespace gui {
namespace fs = std::filesystem;
namespace M = spirula::i18n::msg::survey;

void PoseSurveyPanel::Table::preview() {
    columns.clear();
    error.clear();
    if (input.file.empty()) return;
    try {
        columns = app::survey::previewColumns(input.file, input.header);
        if (!input.header) return;
        const std::array<std::vector<std::string>, 8> names{{
            {"id", "frame_id", "image_id", "image", "filename", "name"},
            {"qx", "q_x"}, {"qy", "q_y"}, {"qz", "q_z"}, {"qw", "q_w"},
            {"x", "tx", "position_x"}, {"y", "ty", "position_y"}, {"z", "tz", "position_z"}}};
        for (size_t i = 0; i < columns.size(); ++i) {
            std::string lower = columns[i];
            for (char& c : lower) c = char(std::tolower(static_cast<unsigned char>(c)));
            for (size_t j = 0; j < names.size(); ++j)
                if (std::find(names[j].begin(), names[j].end(), lower) != names[j].end()) input.columns[j] = int(i);
        }
    } catch (const std::exception& e) {
        error = e.what();
    }
}

bool PoseSurveyPanel::pathRow(const ui::Msg& label, std::string& path, FileDialog::Mode mode,
                              const std::vector<std::string>& extensions, bool relative) {
    ImGui::PushID(&path);
    ui::Text(label);
    ImGui::SetNextItemWidth(-ImGui::CalcTextSize(M::browse.get()).x -
                            2.0f * ImGui::GetStyle().FramePadding.x - ImGui::GetStyle().ItemSpacing.x);
    ui::InputTextRaw("##path", &path);
    const bool edited = ImGui::IsItemDeactivatedAfterEdit();
    ImGui::SameLine();
    if (ui::Button(M::browse)) {
        picker_.open(label.get(), mode, extensions, images_);
        pick_target_ = &path;
        relative_pick_ = relative;
    }
    ImGui::PopID();
    return edited;
}

void PoseSurveyPanel::drawButton(const std::string& workspace, bool disabled) {
    ImGui::BeginDisabled(disabled);
    if (ui::Button(M::title)) {
        open_ = true;
        if (!workspace.empty() && images_.empty()) {
            std::error_code ec;
            const fs::path root = fs::u8path(workspace);
            if (fs::is_directory(root / "images", ec)) images_ = (root / "images").u8string();
            if (fs::is_directory(root / "masks", ec)) masks_ = (root / "masks").u8string();
        }
    }
    ImGui::EndDisabled();
}

void PoseSurveyPanel::start() {
    std::vector<std::string> args{app::exe_path(), "sfm", "survey", "--images", images_, "--output", output_};
    for (const auto& row : tables_) {
        const auto& t = row.input;
        args.insert(args.end(), {"--poses", t.file, t.camera.empty() ? "." : t.camera,
                                 t.header ? "1" : "0", t.world_to_camera ? "1" : "0"});
        for (int col : t.columns) args.push_back(std::to_string(col));
    }
    if (!masks_.empty()) args.insert(args.end(), {"--masks", masks_});
    if (!scan_.empty()) args.insert(args.end(), {"--scan", scan_});
    if (plane_ && !scan_.empty()) args.push_back("--plane-ba");
    if (!transform_.empty()) args.insert(args.end(), {"--rig", first_, second_, transform_});
    if (!intrinsics_.empty()) args.insert(args.end(), {"--intrinsics", intrinsics_});
    if (!flat_.empty()) {
        args.insert(args.end(), {"--flat", flat_});
        if (replace_) args.insert(args.end(), {"--replace", std::to_string(overlap_ / 100.0f)});
    }
    if (intrinsics_.empty() && focal_ > 0) args.insert(args.end(), {"--focal", std::to_string(focal_)});
    log_.clear();
    status_.clear();
    runner_.start(args);
}

void PoseSurveyPanel::draw() {
    for (auto& line : runner_.drain_log()) log_.push_back(std::move(line));
    if (log_.size() > 400) log_.erase(log_.begin(), log_.end() - 400);
    const int result = runner_.take_exit_code();
    if (result != CommandRunner::kNoResult)
        status_ = result == 0 ? std::string(M::done.get()) + "\n" + (fs::u8path(output_) / "sparse" / "0").u8string() +
                               "\n" + (fs::u8path(output_) / app::survey::kReportFile).u8string()
                             : spirula::i18n::format(M::failed, {result});
    if (picker_.draw() && pick_target_) {
        *pick_target_ = picker_.result();
        if (relative_pick_) {
            const auto relative = fs::u8path(*pick_target_).lexically_relative(fs::u8path(images_));
            if (!relative.empty()) *pick_target_ = relative.generic_u8string();
        }
        for (auto& table : tables_) if (pick_target_ == &table.input.file) table.preview();
    }
    if (!picker_.is_open()) pick_target_ = nullptr;
    if (!open_) return;
    ImGui::SetNextWindowSize(ImVec2(660, 800), ImGuiCond_FirstUseEver);
    const std::string title = std::string(M::title.get()) + "###pose-survey";
    if (!ImGui::Begin(title.c_str(), runner_.busy() ? nullptr : &open_)) {
        ImGui::End();
        return;
    }
    ImGui::BeginDisabled(runner_.busy() || picker_.is_open());
    pathRow(M::images, images_, FileDialog::Mode::Folder);
    pathRow(M::output, output_, FileDialog::Mode::Folder);
    pathRow(M::masks, masks_, FileDialog::Mode::Folder);
    ImGui::BeginDisabled(!intrinsics_.empty());
    ui::InputFloat(M::focal, &focal_, 0, 0, "%.1f");
    ImGui::EndDisabled();

    bool valid = !images_.empty() && !output_.empty() && (!intrinsics_.empty() || focal_ >= 0);
    int remove = -1;
    for (size_t i = 0; i < tables_.size(); ++i) {
        Table& row = tables_[i];
        ImGui::PushID(int(i));
        ImGui::Separator();
        if (pathRow(M::pose_file, row.input.file, FileDialog::Mode::File, {".csv", ".txt", ".tsv"})) row.preview();
        if (ui::Checkbox(M::header, &row.input.header)) row.preview();
        int direction = row.input.world_to_camera ? 1 : 0;
        if (ui::ComboRaw("##direction", &direction, {&M::camera_to_world, &M::world_to_camera}))
            row.input.world_to_camera = direction == 1;
        pathRow(M::camera, row.input.camera, FileDialog::Mode::Folder, {}, true);
        if (!row.error.empty()) ui::TextWrappedRaw(row.error);
        valid = valid && row.error.empty() && !row.input.file.empty() && row.columns.size() >= 8;
        if (row.columns.size() >= 8) {
            const char* roles[] = {"", "qx", "qy", "qz", "qw", "x", "y", "z"};
            if (ImGui::BeginTable("##mapping", 2, ImGuiTableFlags_SizingStretchSame)) {
                for (int k = 0; k < 8; ++k) {
                    ImGui::TableNextColumn();
                    int& col = row.input.columns[k];
                    valid = valid && col >= 0 && size_t(col) < row.columns.size();
                    const char* preview = col >= 0 && size_t(col) < row.columns.size() ? row.columns[col].c_str() : "";
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.72f);
                    if (k == 0 ? ui::BeginCombo(M::image_id, preview) : ui::BeginComboRaw(roles[k], preview)) {
                        for (size_t j = 0; j < row.columns.size(); ++j) {
                            ImGui::PushID(int(j));
                            if (ui::SelectableRaw(row.columns[j], col == int(j))) col = int(j);
                            ImGui::PopID();
                        }
                        ImGui::EndCombo();
                    }
                }
                ImGui::EndTable();
            }
            valid = valid && std::set<int>(row.input.columns.begin(), row.input.columns.end()).size() == 8;
        }
        if (tables_.size() > 1 && ui::Button(M::remove_pose) && !picker_.is_open()) remove = int(i);
        ImGui::PopID();
    }
    if (remove >= 0) tables_.erase(tables_.begin() + remove);
    if (ui::Button(M::add_pose) && !picker_.is_open()) tables_.emplace_back();
    ImGui::Separator();
    pathRow(M::scan, scan_, FileDialog::Mode::File, {".las", ".laz", ".e57", ".ply"});
    ImGui::BeginDisabled(scan_.empty());
    ui::Checkbox(M::plane, &plane_);
    ImGui::EndDisabled();
    ui::TextWrapped(M::rig_help);
    pathRow(M::rig, transform_, FileDialog::Mode::File, {".txt", ".csv"});
    if (!transform_.empty()) {
        ui::InputText(M::first, &first_);
        ui::InputText(M::second, &second_);
        valid = valid && !first_.empty() && !second_.empty() && first_ != second_;
    }
    ui::TextWrapped(M::intrinsics_help);
    ui::TextRaw(app::survey::kIntrinsicsExample);
    if (ui::SmallButton(M::intrinsics_copy)) ImGui::SetClipboardText(app::survey::kIntrinsicsExample);
    pathRow(M::intrinsics, intrinsics_, FileDialog::Mode::File, {".txt"});
    ImGui::BeginDisabled(images_.empty());
    pathRow(M::flat, flat_, FileDialog::Mode::Folder, {}, true);
    ImGui::EndDisabled();
    if (!flat_.empty()) {
        ui::Checkbox(M::replace, &replace_);
        if (replace_) ui::SliderFloat(M::overlap, &overlap_, 50, 100, "%.0f%%");
    }
    ImGui::BeginDisabled(!valid || picker_.is_open());
    if (ui::Button(M::run)) start();
    ImGui::EndDisabled();
    ImGui::EndDisabled();
    if (runner_.busy() && ui::Button(M::cancel)) runner_.cancel();
    if (!status_.empty()) ui::TextWrappedRaw(status_);
    if (!log_.empty()) {
        ImGui::BeginChild("##survey-log", ImVec2(0, 175), ImGuiChildFlags_Borders);
        const bool bottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY();
        for (const auto& line : log_) ui::TextRaw(line);
        if (bottom) ImGui::SetScrollHereY(1.0f);
        ImGui::EndChild();
    }
    ImGui::End();
}

}  // namespace gui
#endif
