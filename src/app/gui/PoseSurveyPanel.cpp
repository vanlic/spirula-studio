#include "app/gui/PoseSurveyPanel.h"

#ifdef SS_TOOL_SFM
#include "app/AppPaths.h"
#include "i18n/catalog/PoseSurvey.h"

#include "imgui.h"
#include "imgui_stdlib.h"

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

bool PoseSurveyPanel::pathRow(const char* label, std::string& path, FileDialog::Mode mode,
                              const std::vector<std::string>& extensions, bool relative) {
    ImGui::PushID(&path);
    ImGui::TextUnformatted(label);
    ImGui::SetNextItemWidth(-ImGui::CalcTextSize(M::browse.get()).x -
                            2.0f * ImGui::GetStyle().FramePadding.x - ImGui::GetStyle().ItemSpacing.x);
    ImGui::InputText("##path", &path);
    const bool edited = ImGui::IsItemDeactivatedAfterEdit();
    ImGui::SameLine();
    if (ImGui::Button(M::browse.get())) {
        picker_.open(label, mode, extensions, images_);
        pick_target_ = &path;
        relative_pick_ = relative;
    }
    ImGui::PopID();
    return edited;
}

void PoseSurveyPanel::drawButton(const std::string& workspace, bool disabled) {
    ImGui::BeginDisabled(disabled);
    const std::string button = std::string(M::title.get()) + "###pose_survey_open";
    if (ImGui::Button(button.c_str())) {
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
    pathRow(M::images.get(), images_, FileDialog::Mode::Folder);
    pathRow(M::output.get(), output_, FileDialog::Mode::Folder);
    pathRow(M::masks.get(), masks_, FileDialog::Mode::Folder);
    ImGui::BeginDisabled(!intrinsics_.empty());
    ImGui::InputFloat(M::focal.get(), &focal_, 0, 0, "%.1f");
    ImGui::EndDisabled();

    bool valid = !images_.empty() && !output_.empty() && (!intrinsics_.empty() || focal_ >= 0);
    int remove = -1;
    for (size_t i = 0; i < tables_.size(); ++i) {
        Table& row = tables_[i];
        ImGui::PushID(int(i));
        ImGui::Separator();
        if (pathRow(M::pose_file.get(), row.input.file, FileDialog::Mode::File, {".csv", ".txt", ".tsv"})) row.preview();
        if (ImGui::Checkbox(M::header.get(), &row.input.header)) row.preview();
        int direction = row.input.world_to_camera ? 1 : 0;
        const char* directions[] = {M::camera_to_world.get(), M::world_to_camera.get()};
        if (ImGui::Combo("##direction", &direction, directions, 2)) row.input.world_to_camera = direction == 1;
        pathRow(M::camera.get(), row.input.camera, FileDialog::Mode::Folder, {}, true);
        if (!row.error.empty()) ImGui::TextWrapped("%s", row.error.c_str());
        valid = valid && row.error.empty() && !row.input.file.empty() && row.columns.size() >= 8;
        if (row.columns.size() >= 8) {
            const char* roles[] = {M::image_id.get(), "qx", "qy", "qz", "qw", "x", "y", "z"};
            if (ImGui::BeginTable("##mapping", 2, ImGuiTableFlags_SizingStretchSame)) {
                for (int k = 0; k < 8; ++k) {
                    ImGui::TableNextColumn();
                    int& col = row.input.columns[k];
                    valid = valid && col >= 0 && size_t(col) < row.columns.size();
                    const char* preview = col >= 0 && size_t(col) < row.columns.size() ? row.columns[col].c_str() : "";
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.72f);
                    if (ImGui::BeginCombo(roles[k], preview)) {
                        for (size_t j = 0; j < row.columns.size(); ++j) {
                            ImGui::PushID(int(j));
                            if (ImGui::Selectable(row.columns[j].c_str(), col == int(j))) col = int(j);
                            ImGui::PopID();
                        }
                        ImGui::EndCombo();
                    }
                }
                ImGui::EndTable();
            }
            valid = valid && std::set<int>(row.input.columns.begin(), row.input.columns.end()).size() == 8;
        }
        if (tables_.size() > 1 && ImGui::Button(M::remove_pose.get()) && !picker_.is_open()) remove = int(i);
        ImGui::PopID();
    }
    if (remove >= 0) tables_.erase(tables_.begin() + remove);
    if (ImGui::Button(M::add_pose.get()) && !picker_.is_open()) tables_.emplace_back();
    ImGui::Separator();
    pathRow(M::scan.get(), scan_, FileDialog::Mode::File, {".las", ".laz", ".e57", ".ply"});
    ImGui::BeginDisabled(scan_.empty());
    ImGui::Checkbox(M::plane.get(), &plane_);
    ImGui::EndDisabled();
    ImGui::TextWrapped("%s", M::rig_help.get());
    pathRow(M::rig.get(), transform_, FileDialog::Mode::File, {".txt", ".csv"});
    if (!transform_.empty()) {
        ImGui::InputText(M::first.get(), &first_);
        ImGui::InputText(M::second.get(), &second_);
        valid = valid && !first_.empty() && !second_.empty() && first_ != second_;
    }
    ImGui::TextWrapped("%s", M::intrinsics_help.get());
    ImGui::TextUnformatted(app::survey::kIntrinsicsExample);
    if (ImGui::SmallButton(M::intrinsics_copy.get())) ImGui::SetClipboardText(app::survey::kIntrinsicsExample);
    pathRow(M::intrinsics.get(), intrinsics_, FileDialog::Mode::File, {".txt"});
    ImGui::BeginDisabled(images_.empty());
    pathRow(M::flat.get(), flat_, FileDialog::Mode::Folder, {}, true);
    ImGui::EndDisabled();
    if (!flat_.empty()) {
        ImGui::Checkbox(M::replace.get(), &replace_);
        if (replace_) ImGui::SliderFloat(M::overlap.get(), &overlap_, 50, 100, "%.0f%%");
    }
    ImGui::BeginDisabled(!valid || picker_.is_open());
    const std::string run = std::string(M::run.get()) + "###pose_survey_run";
    if (ImGui::Button(run.c_str())) start();
    ImGui::EndDisabled();
    ImGui::EndDisabled();
    if (runner_.busy() && ImGui::Button(M::cancel.get())) runner_.cancel();
    if (!status_.empty()) ImGui::TextWrapped("%s", status_.c_str());
    if (!log_.empty()) {
        ImGui::BeginChild("##survey-log", ImVec2(0, 175), ImGuiChildFlags_Borders);
        const bool bottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY();
        for (const auto& line : log_) ImGui::TextUnformatted(line.c_str());
        if (bottom) ImGui::SetScrollHereY(1.0f);
        ImGui::EndChild();
    }
    ImGui::End();
}

}  // namespace gui
#endif
