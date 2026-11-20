#pragma once

#ifdef SS_TOOL_SFM
#include "app/PoseSurvey.h"
#include "app/gui/CommandRunner.h"
#include "app/gui/FileDialog.h"

namespace gui {

class PoseSurveyPanel {
public:
    void drawButton(const std::string& workspace, bool disabled);
    void draw();
    bool busy() const { return runner_.busy(); }

private:
    struct Table {
        app::survey::PoseTable input;
        std::vector<std::string> columns;
        std::string error;
        void preview();
    };
    bool pathRow(const char* label, std::string& path, FileDialog::Mode mode,
                 const std::vector<std::string>& extensions = {}, bool relative = false);
    void start();

    bool open_ = false, plane_ = false, replace_ = false;
    float focal_ = 0, overlap_ = 80;
    std::string images_, output_, masks_, scan_, transform_, intrinsics_, flat_;
    std::string first_ = "0", second_ = "1", status_;
    std::vector<Table> tables_{1};
    std::vector<std::string> log_;
    FileDialog picker_;
    std::string* pick_target_ = nullptr;
    bool relative_pick_ = false;
    CommandRunner runner_;
};

}  // namespace gui
#endif
